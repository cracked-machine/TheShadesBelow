#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteOffset.hpp>
#include <Components/AbsoluteRenderOffset.hpp>
#include <Components/AbsoluteRotation.hpp>
#include <Components/AbsoluteScale.hpp>
#include <Components/BurrCount.hpp>
#include <Components/Npc/Ghost.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/Wisp.hpp>
#include <Components/Persistent/BurrExpiry.hpp>
#include <Components/Persistent/BurrThrowCooldown.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Weapons/Burr.hpp>
#include <Components/Weapons/InFlight.hpp>
#include <Events/AddInventoryEvent.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/ItemPickedUpEvent.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/BurrSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Utils/Cardinal.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <limits>

namespace Game::Sys
{

namespace
{

//! @brief The grid cell containing `point`
Cmp::Position cell_at( sf::Vector2f point )
{
  return Cmp::Position( Utils::snap_to_grid( point - Constants::kGridSizePxF / 2.f ), Constants::kGridSizePxF );
}

//! @brief Where a burr sticks to a face of `target`: its impact offset along the face is kept (clamped to the face),
//! and it is moved out onto the face.
//! @param target Bounds of the NPC/obstacle hit.
//! @param impact Centre of the burr when it hit.
//! @param side The face hit, pointing outwards.
//! @return sf::Vector2f The stuck burr's centre, relative to the centre of `target`.
sf::Vector2f stick_offset( const sf::FloatRect &target, sf::Vector2f impact, Utils::Cardinal side )
{
  const sf::Vector2f outwards = side.vector();
  const sf::Vector2f along_face{ -outwards.y, outwards.x };
  const float half_face = along_face.componentWiseMul( target.size / 2.f ).length();
  const float impact_offset = std::clamp( ( impact - target.getCenter() ).dot( along_face ), -half_face, half_face );
  return outwards.componentWiseMul( target.size / 2.f ) + along_face * impact_offset;
}

} // namespace

BurrSystem::BurrSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&BurrSystem::on_player_action_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::AddInventoryEvent>().connect<&BurrSystem::on_add_inventory_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::ItemPickedUpEvent>().connect<&BurrSystem::on_item_picked_up_event>( this );
}

void BurrSystem::on_add_inventory_event( const Events::AddInventoryEvent &ev )
{
  if ( ev.m_item == "item.burr" ) m_throw_cooldown_timer = sf::Time::Zero;
}

void BurrSystem::on_item_picked_up_event( const Events::ItemPickedUpEvent &ev )
{
  if ( ev.m_item == "item.burr" ) m_throw_cooldown_timer = sf::Time::Zero;
}

void BurrSystem::on_player_action_event( Game::Events::PlayerActionEvent ev )
{
  // before the inventory checks, so a charge is abandoned even if the burr has since left the inventory
  if ( ev.action == Game::Events::PlayerActionEvent::GameActions::CANCEL_BOW )
  {
    m_throw_charging = false;
    m_throw_charge_timer = sf::Time::Zero;
    return;
  }

  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type != "item.burr" ) { return; }

  // e.g. releasing the left mouse button that foraged the burr, or spamming throws: start a fresh charge once it's over
  const sf::Time cooldown = sf::seconds( Sys::PersistSystem::get<Cmp::Persist::BurrThrowCooldown>( reg() ).get_value() );
  if ( m_throw_cooldown_timer < cooldown )
  {
    m_throw_charging = false;
    return;
  }

  if ( ev.action == Game::Events::PlayerActionEvent::GameActions::DRAW_BOW )
  {
    // wind up the throw
    m_throw_charging = true;
    m_throw_charge_timer = sf::Time::Zero;
  }
  else if ( ev.action == Game::Events::PlayerActionEvent::GameActions::RELEASE_BOW )
  {
    // only a press made outside the cooldown can throw
    if ( not m_throw_charging ) return;
    m_throw_charging = false;

    // throw the burr; how far it flies depends on how long the throw was charged
    m_sound_bank.get_effect( "burr_throw" ).play();

    float charge_fraction = std::clamp( m_throw_charge_timer.asSeconds() / Constants::kFullChargeSeconds,
                                        Constants::kMinChargeFraction, 1.f );

    create_player_burr( charge_fraction );
    Factory::Player::destroy_inventory( reg(), "item.burr" );
    m_throw_cooldown_timer = sf::Time::Zero;
  }
}

void BurrSystem::update( sf::Time dt )
{
  m_throw_cooldown_timer += dt;
  if ( m_throw_charging ) { m_throw_charge_timer += dt; }
  update_burr_trajectory( dt );
  check_burr_collisions();
  expire_burrs( dt );
}

void BurrSystem::update_burr_trajectory( sf::Time dt )
{
  for ( auto [burr_entt, burr_cmp, burr_pos_cmp] : reg().view<Cmp::Weapons::Projectiles::Burr, Cmp::Position>().each() )
  {
    if ( not reg().any_of<Cmp::Weapons::Projectiles::InFlight>( burr_entt ) ) continue;
    if ( burr_cmp.m_fixed_time_step_accumulator < burr_cmp.fixed_time_step_max() )
    {
      burr_cmp.m_fixed_time_step_accumulator += dt;
      continue;
    }
    burr_cmp.m_fixed_time_step_accumulator = sf::Time::Zero;

    sf::Vector2f remaining = burr_cmp.m_destination - burr_pos_cmp.getCenter();
    if ( remaining.length() <= burr_cmp.speed() ) { burr_pos_cmp.position = burr_cmp.m_destination - burr_pos_cmp.size / 2.f; }
    else if ( auto direction = Utils::Maths::normalized( remaining ) ) { burr_pos_cmp.position += *direction * burr_cmp.speed(); }

    // tumble while airborne
    static const float kSpinDegreesPerStep = 12.f;
    const auto *rotation_cmp = reg().try_get<Cmp::AbsoluteRotation>( burr_entt );
    const float angle = rotation_cmp ? rotation_cmp->getAngle() : 0.f;
    reg().emplace_or_replace<Cmp::AbsoluteRotation>( burr_entt, std::fmod( angle + kSpinDegreesPerStep, 360.f ) );

    // Lift the sprite along a parabolic arc that peaks halfway between origin and destination, as ArrowSystem does.
    // The underlying Position stays on the straight line so NPC/obstacle hit detection is unaffected.
    static const float kParabolaHeight = 15.f;
    sf::Vector2f flight_vector = burr_cmp.m_destination - burr_cmp.m_origin;
    float total_distance = flight_vector.length();
    if ( total_distance > 0.f )
    {
      float progress = std::clamp( ( burr_pos_cmp.getCenter() - burr_cmp.m_origin ).length() / total_distance, 0.f, 1.f );
      float arc_magnitude = kParabolaHeight * progress * ( 1.f - progress );
      sf::Vector2f arc_offset = std::abs( flight_vector.x ) >= std::abs( flight_vector.y ) ? sf::Vector2f{ 0.f, -arc_magnitude }
                                                                                           : sf::Vector2f{ arc_magnitude, 0.f };
      reg().emplace_or_replace<Cmp::AbsoluteRenderOffset>( burr_entt, arc_offset );
    }
  }
}

void BurrSystem::check_burr_collisions()
{
  //! @brief A burr that hit an obstacle, and the face of the obstacle it stuck to.
  struct ObstacleHit
  {
    entt::entity burr_entt;
    entt::entity obstacle_entt;
    Utils::Cardinal side;
  };

  // gather outcomes first: the registry is modified once the view loops are done
  std::vector<std::pair<entt::entity, entt::entity>> npc_hits; // burr, npc
  std::vector<ObstacleHit> obstacle_hits;
  std::vector<std::pair<entt::entity, Cmp::Position>> missed; // burr, drop cell

  for ( auto [burr_entt, burr_cmp, burr_pos_cmp] : reg().view<Cmp::Weapons::Projectiles::Burr, Cmp::Position>().each() )
  {
    if ( not reg().any_of<Cmp::Weapons::Projectiles::InFlight>( burr_entt ) ) continue;
    const auto burr_hitbox = Cmp::RectBounds::scaled( burr_pos_cmp, 0.5f );

    // stick to the first NPC hit; ghosts and wisps are intangible, as for arrows
    entt::entity hit_npc = entt::null;
    for ( auto [npc_entt, npc_cmp, npc_pos_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position>( entt::exclude<Cmp::Npc::Ghost, Cmp::Npc::Wisp> ).each() )
    {
      if ( not burr_hitbox.findIntersection( npc_pos_cmp ) ) continue;
      hit_npc = npc_entt;
      break;
    }
    if ( hit_npc != entt::null )
    {
      npc_hits.emplace_back( burr_entt, hit_npc );
      continue;
    }

    // check the burr and the grid cell beneath it against anything that blocks the player. Of the blockers the burr
    // touches, the one nearest the last free cell it flew over is the one whose face it flew into.
    const Cmp::Position cell = cell_at( burr_pos_cmp.getCenter() );
    const auto cell_hitbox = Cmp::RectBounds::scaled( cell, 0.5f );
    const sf::Vector2f approach = burr_cmp.m_last_free_cell ? burr_cmp.m_last_free_cell->getCenter() : burr_cmp.m_origin;
    entt::entity hit_obstacle = entt::null;
    float hit_distance = std::numeric_limits<float>::max();
    bool cell_blocked = false;
    for ( auto [blocker_entt, blocker_pos_cmp] : reg().view<Cmp::Player::NoPath, Cmp::Position>().each() )
    {
      if ( cell_hitbox.findIntersection( blocker_pos_cmp ) ) cell_blocked = true;
      if ( not burr_hitbox.findIntersection( blocker_pos_cmp ) ) continue;
      const float distance = ( blocker_pos_cmp.getCenter() - approach ).length();
      if ( distance >= hit_distance ) continue;
      hit_obstacle = blocker_entt;
      hit_distance = distance;
    }

    if ( hit_obstacle != entt::null )
    {
      // thrown while flying over an obstacle: there's no outside face to stick to
      if ( not burr_cmp.m_last_free_cell )
      {
        missed.emplace_back( burr_entt, cell_at( burr_cmp.m_origin ) );
        continue;
      }
      const auto &obstacle_pos_cmp = reg().get<Cmp::Position>( hit_obstacle );
      const auto side = Utils::Cardinal::dominant( burr_cmp.m_last_free_cell->getCenter() - obstacle_pos_cmp.getCenter() );
      obstacle_hits.push_back( { burr_entt, hit_obstacle, side } );
      continue;
    }
    if ( not cell_blocked ) burr_cmp.m_last_free_cell = cell;

    // reached the destination without hitting anything
    if ( ( burr_cmp.m_destination - burr_pos_cmp.getCenter() ).length() > 0.01f ) continue;
    missed.emplace_back( burr_entt, cell_at( burr_cmp.m_destination ) );
  }

  // stick to the NPC face nearest the burr, where it struck; render_attached_burrs() draws it there as the NPC moves
  for ( auto [burr_entt, npc_entt] : npc_hits )
  {
    const auto &npc_pos_cmp = reg().get<Cmp::Position>( npc_entt );
    const sf::Vector2f impact = reg().get<Cmp::Position>( burr_entt ).getCenter();
    const auto side = Utils::Cardinal::dominant( impact - npc_pos_cmp.getCenter() );
    reg().get_or_emplace<Cmp::BurrCount>( npc_entt ).add( side, stick_offset( npc_pos_cmp, impact, side ) );
    reg().destroy( burr_entt );
  }

  // the burr sprite stays stuck on the obstacle face until it expires along with its Cmp::BurrCount entry
  for ( const auto &hit : obstacle_hits )
  {
    // stick where the burr struck
    const auto &obstacle_pos_cmp = reg().get<Cmp::Position>( hit.obstacle_entt );
    auto &burr_pos_cmp = reg().get<Cmp::Position>( hit.burr_entt );
    const sf::Vector2f offset = stick_offset( obstacle_pos_cmp, burr_pos_cmp.getCenter(), hit.side );
    reg().get_or_emplace<Cmp::BurrCount>( hit.obstacle_entt ).add( hit.side, offset );
    const sf::Vector2f burr_centre = obstacle_pos_cmp.getCenter() + offset;

    burr_pos_cmp.position = burr_centre - burr_pos_cmp.size / 2.f;
    reg().get<Cmp::Weapons::Projectiles::Burr>( hit.burr_entt ).m_stuck_to = hit.obstacle_entt;
    reg().remove<Cmp::Weapons::Projectiles::InFlight>( hit.burr_entt );
    reg().remove<Cmp::AbsoluteRenderOffset>( hit.burr_entt );
    reg().emplace_or_replace<Cmp::AbsoluteRotation>( hit.burr_entt, hit.side.angle().asDegrees() );
    reg().emplace_or_replace<Cmp::AbsoluteScale>( hit.burr_entt, Cmp::BurrCount::kStuckScale );
    // just in front of the obstacle; multiblock segments have no z-order of their own, so sort by the burr's bottom edge
    const auto *obstacle_z_cmp = reg().try_get<Cmp::ZOrderValue>( hit.obstacle_entt );
    reg().emplace_or_replace<Cmp::ZOrderValue>( hit.burr_entt,
                                                obstacle_z_cmp ? obstacle_z_cmp->get() + 0.5f : burr_centre.y + burr_pos_cmp.size.y / 4.f );
  }

  // a miss sends the burr back into the world as an item that can be picked up again
  for ( auto [burr_entt, drop_cell] : missed )
  {
    reg().destroy( burr_entt );
    get_systems_event_queue().trigger( Events::CreateItemEvent( drop_cell, Sys::ItemStore::instance().pick_item( "item.burr" ), "drop_loot" ) );
  }
}

void BurrSystem::expire_burrs( sf::Time dt )
{
  const sf::Time expiry = sf::seconds( Sys::PersistSystem::get<Cmp::Persist::BurrExpiry>( reg() ).get_value() );

  // burrs fall off once they expire, or when their obstacle stops blocking (e.g. dug out or bombed)
  std::vector<entt::entity> burr_free;
  for ( auto [entt, burr_count_cmp] : reg().view<Cmp::BurrCount>().each() )
  {
    burr_count_cmp.tick( dt, expiry );
    const bool obstacle_gone = not reg().any_of<Cmp::Npc::NPC, Cmp::Player::NoPath>( entt );
    if ( burr_count_cmp.empty() or obstacle_gone ) burr_free.push_back( entt );
  }
  for ( auto entt : burr_free )
  {
    reg().remove<Cmp::BurrCount>( entt );
  }

  std::vector<entt::entity> spent_burrs;
  for ( auto [burr_entt, burr_cmp] : reg().view<Cmp::Weapons::Projectiles::Burr>( entt::exclude<Cmp::Weapons::Projectiles::InFlight> ).each() )
  {
    burr_cmp.m_stuck_elapsed += dt;
    const bool unstuck = not reg().valid( burr_cmp.m_stuck_to ) or not reg().all_of<Cmp::BurrCount>( burr_cmp.m_stuck_to );
    if ( burr_cmp.m_stuck_elapsed >= expiry or unstuck ) spent_burrs.push_back( burr_entt );
  }
  for ( auto burr_entt : spent_burrs )
  {
    reg().destroy( burr_entt );
  }
}

void BurrSystem::create_player_burr( float charge_fraction )
{
  auto mouse_pos = Utils::get_mouse_bounds_in_gameview( m_window, RenderSystem::get_world_view() );
  SPDLOG_INFO( "Throwing burr to {},{}", mouse_pos.position.x, mouse_pos.position.y );

  // Charge scales how far along the origin->mouse line the burr actually travels;
  // a weak throw drops the burr short instead of always reaching the cursor.
  sf::Vector2f origin_center = Utils::Player::get_position( reg() ).getCenter();
  sf::Vector2f burr_destination = origin_center + ( mouse_pos.getCenter() - origin_center ) * charge_fraction;

  auto burr_entt = reg().create();
  reg().emplace_or_replace<Cmp::Position>( burr_entt, origin_center - Constants::kGridSizePxF / 2.f, Constants::kGridSizePxF );
  reg().emplace_or_replace<Cmp::Weapons::Projectiles::InFlight>( burr_entt );
  reg().emplace_or_replace<Cmp::Weapons::Projectiles::Burr>( burr_entt, origin_center, burr_destination );
  reg().emplace_or_replace<Cmp::ZOrderValue>( burr_entt, 50000 );
  reg().emplace_or_replace<Cmp::AbsoluteOffset>( burr_entt, Constants::kGridSizePxF / 2.f );
  // clang-format off
  reg().emplace_or_replace<Cmp::AnimData>( burr_entt, Cmp::AnimData::Config{
        .sprite_type = "sprite.burr",
        .frame_index_offset = 0,
        .enabled = true
  });
  // clang-format on
}

} // namespace Game::Sys
