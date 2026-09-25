#include <Audio/SoundBank.hpp>
#include <Components/Persistent/LightningDamage.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/UUID.hpp>
#include <Events/LightningEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Threats/LightningSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Player.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Vertex.hpp>

namespace Game::Sys
{

LightningSystem::LightningSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory,
                                  Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank )
{
  // retrieve a sink handle for the event, register the member function as an event listener and keep the connection alive (ignore returned object)
  std::ignore = get_systems_event_queue().sink<Events::LightningEvent>().connect<&Sys::LightningSystem::on_lightning_event>( this );
}

void LightningSystem::on_lightning_event( [[maybe_unused]] Events::LightningEvent ev ) { trigger_lightning = true; };

void LightningSystem::update( sf::Time dt )
{
  if ( trigger_lightning and not lightning_strike_exists() )
  {
    create_lightning_strike( dt );
    auto uuid = Cmp::UUID::generate();
    Factory::Particle::add_smoke( reg(), "player.lightning.particle.smoke", uuid, Utils::Player::get_position( reg() ).getCenter(), 50000 );
    trigger_lightning = false;

    int lightning_dmg = Sys::PersistSystem::get<Cmp::Persist::LightningDamage>( reg() ).get_value();
    Cmp::BaseAction lightning_action( Cmp::Stats::Health{ -lightning_dmg }, {}, {}, {}, {}, {} );
    Utils::Player::get_stats( reg() ).apply( lightning_action );

    if ( Utils::Player::get_stats( reg() ).health() <= 0 )
    {
      get_systems_event_queue().trigger(
          Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::SHOCKED, Utils::Player::get_position( reg() ) ) );
    }
  }

  delete_expired_lightning_strikes();
  Factory::Particle::delete_expired_particle_sprites( reg(), "player.lightning.particle.smoke" );

  // update the position so that it follows player
  sf::Vector2f new_pos( Utils::Player::get_position( reg() ).x() + 8.f, Utils::Player::get_position( reg() ).y() );
  Factory::Particle::update_position( reg(), "player.lightning.particle.smoke", new_pos );
}

bool LightningSystem::lightning_strike_exists()
{
  auto ls_view = reg().view<Cmp::LightningStrike>();
  return not ls_view.empty();
}

void LightningSystem::create_lightning_strike( [[maybe_unused]] sf::Time dt )
{
  auto player_position = Utils::Player::get_position( reg() );

  // make sure the origin is within the game view (not world y=0!)
  auto origin_pos = sf::Vector2f( player_position.getCenter().x, player_position.y() - ( RenderSystem::get_world_view().getSize().y / 2 ) );

  auto lightning_strikes_count = 5;
  auto lightning_segment_divisor = 5; // 1 = 2 segments, 2 = 4 segment, 3 = 8 segments, 4 = 16 segments
  auto angle_deviations = Cmp::LightningStrike::AngleDeviations{ .inner = 8.f, .outer = 8.f };
  auto strike_duration = sf::seconds( 0.1 );
  auto max_lines_per_strike = 5;

  for ( auto _ : std::views::iota( 0, lightning_strikes_count ) )
  {
    Cmp::LightningStrike ls_cmp_main( origin_pos, player_position.getCenter(), angle_deviations, strike_duration );

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
    if ( cmp.timer.getElapsedTime() >= cmp.duration ) { kill_list.push_back( entt ); }
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
