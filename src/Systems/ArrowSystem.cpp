#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteOffset.hpp>
#include <Components/AbsoluteRenderOffset.hpp>
#include <Components/AbsoluteRotation.hpp>
#include <Components/Npc/Container.hpp>
#include <Components/Npc/Ghost.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Random.hpp>
#include <Components/Weapons/Arrow.hpp>
#include <Components/Weapons/InFlight.hpp>
#include <Factory/NpcFactory.hpp>
#include <SFML/System/Angle.hpp>
#include <Systems/ArrowSystem.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

namespace Game::Sys
{

ArrowSystem::ArrowSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&ArrowSystem::on_player_action_event>( this );
}

void ArrowSystem::on_player_action_event( Game::Events::PlayerActionEvent ev )
{
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type != "item.bow" ) { return; }

  if ( ev.action == Game::Events::PlayerActionEvent::GameActions::DRAW_BOW )
  {
    // draw the bow

    if ( m_sound_bank.get_effect( "draw_bow" ).getStatus() != sf::Sound::Status::Playing ) m_sound_bank.get_effect( "draw_bow" ).play();
    m_bow_drawing = true;
    m_bow_draw_clock.restart();
  }
  else if ( ev.action == Game::Events::PlayerActionEvent::GameActions::RELEASE_BOW )
  {
    // release an arrow; how far it flies depends on how long the bow was held drawn
    m_sound_bank.get_effect( "release_bow" ).play();

    static const float kMinChargeFraction = 0.25f;
    static const float kFullDrawSeconds = 1.f;
    float charge_fraction = m_bow_drawing ? std::clamp( m_bow_draw_clock.getElapsedTime().asSeconds() / kFullDrawSeconds, kMinChargeFraction, 1.f )
                                          : 1.f;
    m_bow_drawing = false;

    create_player_arrow( charge_fraction );
  }
}

void ArrowSystem::update( sf::Time dt )
{
  update_arrow_trajectory( dt );
  check_npc_arrow_collision();
}

void ArrowSystem::update_arrow_trajectory( sf::Time dt )
{

  for ( auto [arrow_entt, arrow_cmp, arrow_pos_cmp] : reg().view<Cmp::Weapons::Projectiles::Arrow, Cmp::Position>().each() )
  {
    if ( arrow_cmp.m_fixed_time_step_accumulator >= arrow_cmp.fixed_time_step_max() )
    {
      if ( reg().any_of<Cmp::Weapons::Projectiles::InFlight>( arrow_entt ) )
      {
        // Arrow still in flight; update the arrow position/angle using its latest direction
        sf::Vector2f remaining = arrow_cmp.m_destination - arrow_pos_cmp.getCenter();
        if ( remaining.length() <= arrow_cmp.speed() ) { arrow_pos_cmp.position = arrow_cmp.m_destination - arrow_pos_cmp.size / 2.f; }
        else if ( auto direction = Utils::Maths::normalized( remaining ) )
        {
          arrow_pos_cmp.position += *direction * arrow_cmp.speed();
          reg().emplace_or_replace<Cmp::AbsoluteRotation>( arrow_entt, Utils::Maths::angle( *direction ).value_or( sf::Angle::Zero ).asDegrees() );
        }

        // Displace the sprite along a parabolic arc that peaks halfway between origin and destination.
        // The underlying Position stays on the straight line so NPC hit detection is unaffected.
        // The arc is drawn perpendicular to the flight path: a vertical bulge for a mostly-horizontal
        // shot, a horizontal bulge for a mostly-vertical shot, so it stays visible either way.
        static const float kParabolaHeight = 15.f;
        sf::Vector2f flight_vector = arrow_cmp.m_destination - arrow_cmp.m_origin;
        float total_distance = flight_vector.length();
        if ( total_distance > 0.f )
        {
          float progress = std::clamp( ( arrow_pos_cmp.getCenter() - arrow_cmp.m_origin ).length() / total_distance, 0.f, 1.f );
          float arc_magnitude = kParabolaHeight * progress * ( 1.f - progress );
          sf::Vector2f arc_offset = std::abs( flight_vector.x ) >= std::abs( flight_vector.y ) ? sf::Vector2f{ 0.f, arc_magnitude }
                                                                                               : sf::Vector2f{ arc_magnitude, 0.f };
          reg().emplace_or_replace<Cmp::AbsoluteRenderOffset>( arrow_entt, arc_offset );
        }
      }

      // Checked unconditionally (not just when already landed) so that the impact sound/effects fire
      // on the exact same tick the arrow snaps to its destination above, instead of one tick later.
      if ( arrow_pos_cmp.getCenter() == arrow_cmp.m_destination )
      {
        // arrow has reached destination
        if ( reg().any_of<Cmp::Weapons::Projectiles::InFlight>( arrow_entt ) )
        {
          m_sound_bank.get_effect( std::string( "arrow_hit" + std::to_string( Cmp::RandomInt( 1, 4 ).gen() ) ) ).play();
          m_sound_bank.get_effect( "draw_bow" ).stop();
          m_sound_bank.get_effect( "release_bow" ).stop();
          reg().remove<Cmp::Weapons::Projectiles::InFlight>( arrow_entt );

          if ( auto *rotation_cmp = reg().try_get<Cmp::AbsoluteRotation>( arrow_entt ) ) arrow_cmp.m_rest_angle = rotation_cmp->getAngle();

          // Re-pivot rotation around the arrowhead (tip) instead of the sprite centre, so the impact
          // point stays fixed in place while the fletching end wiggles around it. AbsoluteOffset is a
          // local pivot in sprite space (tip = far edge along the sprite's default facing direction);
          // AbsoluteRenderOffset compensates in world space so the tip lands exactly where the sprite's
          // centre used to be, accounting for whatever angle the arrow was flying at on impact -
          // otherwise the sprite visibly snaps sideways the instant it switches pivot.
          float rest_rad = sf::degrees( arrow_cmp.m_rest_angle ).asRadians();
          float half_width = arrow_pos_cmp.size.x / 2.f;
          sf::Vector2f tip_from_centre{ std::cos( rest_rad ) * half_width, std::sin( rest_rad ) * half_width };
          reg().emplace_or_replace<Cmp::AbsoluteOffset>( arrow_entt, sf::Vector2f{ arrow_pos_cmp.size.x, arrow_pos_cmp.size.y / 2.f } );
          reg().emplace_or_replace<Cmp::AbsoluteRenderOffset>( arrow_entt, tip_from_centre - sf::Vector2f{ half_width, 0.f } );
        }

        arrow_cmp.m_landed_elapsed += dt;

        // Destroy spent arrows a while after they land so they don't pile up.
        static const sf::Time kDestroyDelay = sf::seconds( 5.f );
        if ( arrow_cmp.m_landed_elapsed >= kDestroyDelay )
        {
          reg().destroy( arrow_entt );
          continue;
        }

        // Wiggle the arrow briefly on impact, decaying back to its resting angle.
        static const sf::Time kWiggleDuration = sf::seconds( 0.2f );
        static const float kWiggleFrequencyHz = 100.f;
        static const float kWiggleAmplitudeDeg = 1.f;
        if ( arrow_cmp.m_landed_elapsed < kWiggleDuration )
        {
          float decay = 1.f - ( arrow_cmp.m_landed_elapsed.asSeconds() / kWiggleDuration.asSeconds() );
          float wiggle_deg = kWiggleAmplitudeDeg * decay * std::sin( arrow_cmp.m_landed_elapsed.asSeconds() * kWiggleFrequencyHz );
          reg().emplace_or_replace<Cmp::AbsoluteRotation>( arrow_entt, arrow_cmp.m_rest_angle + wiggle_deg );
        }
      }

      arrow_cmp.m_fixed_time_step_accumulator = sf::Time::Zero;
    }
    else { arrow_cmp.m_fixed_time_step_accumulator += dt; }
  }
}

void ArrowSystem::check_npc_arrow_collision()
{
  for ( auto [arrow_entt, arrow_cmp, arrow_pos_cmp] : reg().view<Cmp::Weapons::Projectiles::Arrow, Cmp::Position>().each() )
  {
    for ( auto [npc_entt, npc_cmp, npc_pos_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position>().each() )
    {
      if ( not reg().any_of<Cmp::Weapons::Projectiles::InFlight>( arrow_entt ) ) continue;
      if ( reg().any_of<Cmp::Npc::Ghost>( npc_entt ) ) continue;
      if ( not npc_pos_cmp.findIntersection( arrow_pos_cmp ) ) continue;
      Factory::Npc::destroy_npc( reg(), npc_entt );
    }
    for ( auto [npc_entt, npc_cmp, npc_pos_cmp] : reg().view<Cmp::Npc::Container, Cmp::Position>().each() )
    {
      if ( not npc_pos_cmp.findIntersection( arrow_pos_cmp ) ) continue;
      Factory::Npc::destroy_npc_container( reg(), npc_entt );
    }
  }
}

void ArrowSystem::create_player_arrow( float charge_fraction )
{
  auto mouse_pos = Utils::get_mouse_bounds_in_gameview( m_window, RenderSystem::get_world_view() );
  SPDLOG_INFO( "Firing arrow to {},{}", mouse_pos.position.x, mouse_pos.position.y );

  auto arrow_entt = reg().create();
  auto arrow_origin = Utils::Player::get_position( reg() );
  reg().emplace_or_replace<Cmp::Position>( arrow_entt, arrow_origin.position, arrow_origin.size );

  // Charge scales how far along the origin->mouse line the arrow actually travels;
  // a weak draw drops the arrow short instead of always reaching the cursor.
  sf::Vector2f origin_center = arrow_origin.getCenter();
  sf::Vector2f arrow_destination = origin_center + ( mouse_pos.getCenter() - origin_center ) * charge_fraction;

  reg().emplace_or_replace<Cmp::Weapons::Projectiles::InFlight>( arrow_entt );
  reg().emplace_or_replace<Cmp::Weapons::Projectiles::Arrow>( arrow_entt, origin_center, arrow_destination );
  reg().emplace_or_replace<Cmp::ZOrderValue>( arrow_entt, 50000 );
  reg().emplace_or_replace<Cmp::AbsoluteOffset>( arrow_entt, arrow_origin.size / 2.f );
  if ( auto angle = Utils::Maths::angle( mouse_pos.getCenter() - arrow_origin.getCenter() ) )
  {
    reg().emplace_or_replace<Cmp::AbsoluteRotation>( arrow_entt, angle->asDegrees() );
  }
  // clang-format off
    reg().emplace_or_replace<Cmp::AnimData>( arrow_entt, Cmp::AnimData::Config{
          .sprite_type = "sprite.item.arrow",
          .frame_index_offset = 0,
          .enabled = true
    });
  // clang-format on
}

} // namespace Game::Sys