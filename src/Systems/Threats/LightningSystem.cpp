#include <Audio/SoundBank.hpp>
#include <Components/AnimData.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Particle/LightningRodParticleSprite.hpp>
#include <Components/Persistent/LightningDamage.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/TookDamage.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/UUID.hpp>
#include <Events/LightningEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Events/SwapWorldItemEvent.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Threats/LightningSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include <algorithm>
#include <utility>

namespace Game::Sys
{

LightningSystem::LightningSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  // retrieve a sink handle for the event, register the member function as an event listener and keep the connection alive (ignore returned object)
  std::ignore = get_systems_event_queue().sink<Events::LightningEvent>().connect<&Sys::LightningSystem::on_lightning_event>( this );
}

void LightningSystem::on_lightning_event( Events::LightningEvent ev )
{
  m_trigger_lightning = true;
  m_target_entt = ev.target_entt == entt::null ? find_lightning_target() : ev.target_entt;
}

entt::entity LightningSystem::find_lightning_target()
{
  // A working lightning rod on screen takes the strike, otherwise the player is struck.
  for ( auto [worlditem_entt, worlditem_cmp, worlditem_pos_cmp] : reg().view<Cmp::WorldItem, Cmp::Position>().each() )
  {
    if ( worlditem_cmp.item_type != "item.lightningrod" ) continue;
    if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), worlditem_pos_cmp ) ) continue;
    auto *wearlevel = reg().try_get<Cmp::Inventory::WearLevel>( worlditem_entt );
    if ( not wearlevel or wearlevel->m_level <= 0 ) continue;
    return worlditem_entt;
  }
  return Utils::Player::get_entity( reg() );
}

void LightningSystem::update( sf::Time dt )
{
  if ( m_trigger_lightning and not lightning_strike_exists() )
  {
    m_trigger_lightning = false;
    const auto target_entt = std::exchange( m_target_entt, entt::null );

    if ( reg().valid( target_entt ) and reg().any_of<Cmp::Player::Character>( target_entt ) ) { strike_player(); }
    else if ( reg().valid( target_entt ) and reg().any_of<Cmp::WorldItem>( target_entt ) )
    {
      auto *worlditem_pos = reg().try_get<Cmp::Position>( target_entt );
      auto *wearlevel = reg().try_get<Cmp::Inventory::WearLevel>( target_entt );
      const bool is_lightning_rod = reg().get<Cmp::WorldItem>( target_entt ).item_type == "item.lightningrod";

      // a worn out lightning rod is no longer a valid target, so the player takes the strike instead
      if ( is_lightning_rod and worlditem_pos and wearlevel and wearlevel->m_level > 0 )
      {
        float lightning_dmg = Sys::PersistSystem::get<Cmp::Persist::LightningDamage>( reg() ).get_value();
        wearlevel->m_level = std::max( 0.f, wearlevel->m_level - lightning_dmg );
        if ( wearlevel->m_level <= 0 )
        {
          // only one rod is tracked at a time: if another is still glowing, break it now
          if ( m_breaking_rod_entt != entt::null ) break_lightning_rod( m_breaking_rod_entt );

          // play the glowing animation once; the rod breaks in update() once kRodBreakDelay has elapsed
          reg().emplace_or_replace<Cmp::AnimData>(
              target_entt,
              Cmp::AnimData::Config{ .sprite_type = "sprite.item.lightningrod.glowing", .enabled = true, .anim_type = Cmp::AnimType::ONESHOTHOLD } );
          m_breaking_rod_entt = target_entt;
          m_rod_break_elapsed = sf::Time::Zero;
        }
        create_lightning_strike( *worlditem_pos );
        auto uuid = Cmp::UUID::generate();
        Factory::Particle::add_smoke( reg(), "player.lightning.particle.smoke", uuid, worlditem_pos->getCenter(), 50000 );
        m_player_smoke_uuid.reset();
      }
      else if ( is_lightning_rod ) { strike_player(); }
    }
  }

  // update the position so that it follows player, but only if the player was struck
  if ( m_player_smoke_uuid )
  {
    const auto player_pos = Utils::Player::get_position( reg() );
    Factory::Particle::update_position( reg(), *m_player_smoke_uuid, sf::Vector2f( player_pos.x() + 8.f, player_pos.y() ) );
  }

  if ( m_breaking_rod_entt != entt::null )
  {
    m_rod_break_elapsed += dt;
    if ( m_rod_break_elapsed >= kRodBreakDelay ) break_lightning_rod( m_breaking_rod_entt );
  }

  delete_expired_lightning_strikes();
  Factory::Particle::delete_expired_particle_sprites( reg(), "player.lightning.particle.smoke" );
  Factory::Particle::delete_expired_particle_sprites( reg(), std::string( Cmp::Particle::LightningRodParticleSprite::kTag ) );
}

void LightningSystem::break_lightning_rod( entt::entity rod_entt )
{
  m_breaking_rod_entt = entt::null;

  // the rod may have been picked up or destroyed while it was glowing
  if ( not reg().valid( rod_entt ) or not reg().all_of<Cmp::WorldItem, Cmp::Position>( rod_entt ) ) return;
  if ( reg().get<Cmp::WorldItem>( rod_entt ).item_type != "item.lightningrod" ) return;

  // copy the position: the swap event replaces this entity's components
  const Cmp::Position rod_pos = reg().get<Cmp::Position>( rod_entt );

  // speed falls off as lifetime_ratio^4, so a particle travels speed * lifetime / 5:
  // 160px/s over 2s reaches ~4 grid blocks (64px) for the longest-lived particles
  auto rod_particle_uuid = Cmp::UUID::generate();
  Factory::Particle::add_lightningrod_ps( reg(), std::string( Cmp::Particle::LightningRodParticleSprite::kTag ), 20, 2.f, 160.f, 20.f,
                                          rod_particle_uuid, rod_pos.position, rod_pos.position.y );
  m_sound_bank.get_effect( "bomb_detonate" ).play();

  // swap out "item.lightningrod" for "item.lightningrod.broken"
  get_systems_event_queue().trigger( Events::SwapWorldItemEvent( rod_entt, "item.lightningrod.broken", "tool_break" ) );
}

void LightningSystem::strike_player()
{
  const auto player_pos = Utils::Player::get_position( reg() );
  int lightning_dmg = Sys::PersistSystem::get<Cmp::Persist::LightningDamage>( reg() ).get_value();
  Cmp::BaseAction lightning_action( Cmp::Stats::Health{ -lightning_dmg }, {}, {}, {}, {}, {} );
  Utils::Player::get_stats( reg() ).apply( lightning_action );
  reg().emplace_or_replace<Cmp::Player::TookDamage>( Utils::Player::get_entity( reg() ) );

  if ( Utils::Player::get_stats( reg() ).health() <= 0 )
  {
    get_systems_event_queue().trigger( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::SHOCKED, player_pos ) );
  }

  create_lightning_strike( player_pos );
  m_player_smoke_uuid = Cmp::UUID::generate();
  Factory::Particle::add_smoke( reg(), "player.lightning.particle.smoke", *m_player_smoke_uuid, player_pos.getCenter(), 50000 );
}

bool LightningSystem::lightning_strike_exists()
{
  auto ls_view = reg().view<Cmp::LightningStrike>();
  return not ls_view.empty();
}

void LightningSystem::create_lightning_strike( const Cmp::Position &target_pos )
{
  // make sure the origin is within the game view (not world y=0!)
  auto origin_pos = sf::Vector2f( target_pos.getCenter().x, target_pos.y() - ( RenderSystem::get_world_view().getSize().y / 2 ) );

  auto lightning_strikes_count = 5;
  auto lightning_segment_divisor = 5; // 1 = 2 segments, 2 = 4 segment, 3 = 8 segments, 4 = 16 segments
  auto angle_deviations = Cmp::LightningStrike::AngleDeviations{ .inner = 8.f, .outer = 8.f };
  auto strike_duration = sf::seconds( 0.1 );
  auto max_lines_per_strike = 5;

  for ( auto _ : std::views::iota( 0, lightning_strikes_count ) )
  {
    Cmp::LightningStrike ls_cmp_main( origin_pos, target_pos.getCenter(), angle_deviations, strike_duration );

    for ( auto _ : std::views::iota( 0, lightning_segment_divisor ) )
    {
      divide_lightning_segments( ls_cmp_main.sequence, ls_cmp_main.m_deviations, max_lines_per_strike );
    }
    SPDLOG_INFO( "Lightning strike main segment count: {}", ls_cmp_main.sequence.size() - 1 );

    auto entt_main = reg().create();
    reg().emplace_or_replace<Cmp::LightningStrike>( entt_main, ls_cmp_main );
    SPDLOG_INFO( "Created lightning strike {}", static_cast<uint32_t>( entt_main ) );
  }
  m_sound_bank.get_effect( "lightning_strike" ).play();
}

void LightningSystem::delete_expired_lightning_strikes()
{
  std::vector<entt::entity> kill_list;
  for ( auto [entt, cmp] : reg().view<Cmp::LightningStrike>().each() )
  {
    if ( cmp.timer >= cmp.duration ) { kill_list.push_back( entt ); }
  }

  for ( auto &entt : kill_list )
  {
    reg().destroy( entt );
    SPDLOG_INFO( "Destroyed lightning strike {}", static_cast<uint32_t>( entt ) );
  }
}

void LightningSystem::divide_lightning_segments( std::vector<std::vector<sf::Vertex>> &ls_seq, Cmp::LightningStrike::AngleDeviations deviations,
                                                 int lines_per_strike )
{

  if ( ls_seq.size() < 2 )
  {
    SPDLOG_WARN( "Lightning component has empty sequence. Skipping segment division step." );
    return;
  }

  std::vector<std::vector<sf::Vertex>> result;
  result.reserve( ( ls_seq.size() * 2 ) - 1 );
  auto inner_deviation_rng = Cmp::RandomFloat( 0, deviations.inner );
  auto outer_deviation_rng = Cmp::RandomFloat( 0, deviations.outer );
  auto row_size_rng = Cmp::RandomInt( 1, lines_per_strike );

  auto perp_direction_rng = Cmp::RandomInt( 0, 1 );
  int offset_pick = perp_direction_rng.gen();

  for ( auto curr_row_iter = ls_seq.begin(); curr_row_iter < ls_seq.end(); curr_row_iter++ )
  {
    result.push_back( { *curr_row_iter } );
    auto next_row_iter = std::next( curr_row_iter );
    if ( next_row_iter == ls_seq.end() ) { break; }
    if ( next_row_iter->empty() ) { break; }

    std::vector<sf::Vertex> next_row;
    for ( auto idx : std::views::iota( 0, 3 ) )
    {
      // insert new vertex between existing pair
      // use curr_row[0] as the default point for all mid vertices
      sf::Vector2f midpoint = ( curr_row_iter->at( 0 ).position + next_row_iter->at( 0 ).position ) / 2.f;

      // add random perpendicular offset
      sf::Vector2f direction = next_row_iter->at( 0 ).position - curr_row_iter->at( 0 ).position;
      // use length threshold instead of exact zero comparison
      if ( direction.length() < 0.01f ) { continue; }

      sf::Vector2f offsetVector;
      if ( offset_pick == 0 ) { offsetVector = { -direction.y, direction.x }; } // rotate 90 degrees
      else { offsetVector = { direction.y, direction.x }; }                     // rotate -90 degrees
      auto normalized = Utils::Maths::normalized( offsetVector );
      if ( not normalized.has_value() ) { continue; } // normalise the offset to within +/-1

      if ( idx == 0 ) { midpoint += normalized.value() * inner_deviation_rng.gen(); }
      else { midpoint += normalized.value() * outer_deviation_rng.gen(); }

      if ( idx == 0 ) { next_row.push_back( sf::Vertex( midpoint ) ); }
      else { next_row.push_back( sf::Vertex( midpoint ) ); }
    }
    if ( not next_row.empty() ) { result.emplace_back( next_row ); } // only add if we produced vertices
  }

  ls_seq = std::move( result );
}

} // namespace Game::Sys
