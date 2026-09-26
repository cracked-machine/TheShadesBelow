#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Player/TookDamage.hpp>
#include <Components/Stats/PlayerStats.hpp>
#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <SFML/System/Time.hpp>
#include <Shaders/UniformBuilder.hpp>
#include <Systems/PersistSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Player.hpp>

#include <Shaders/RedVignetteShader.hpp>

namespace Game::Sprites
{

namespace
{
// How many "e-foldings" per second the displayed toxicity closes the gap to the real stat by; see
// m_smoothed_toxicity in RedVignetteShader.hpp for why this exists.
constexpr float kToxicitySmoothingRate = 3.0f;
} // namespace

void RedVignetteShader::update( entt::registry &reg, sf::Time dt )
{

  auto display_size = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg ) );

  // TookDamage is consumed here. Each new hit restarts the hold timer, so sparse damage sources (e.g. the once-per-second
  // toxidrome ticks) can't race a free-running timer that would zero the severity right after the tag was added.
  constexpr sf::Time kHoldTime = sf::seconds( 0.3f );
  constexpr float kFadeInRate = 6.f;   // severity per second
  constexpr float kFadeOutRate = 1.5f; // severity per second

  const auto player_entt = Utils::Player::get_entity( reg );
  if ( reg.all_of<Cmp::Player::TookDamage>( player_entt ) )
  {
    reg.remove<Cmp::Player::TookDamage>( player_entt );
    m_update_timer = sf::Time::Zero;
  }

  // fade the effect in/out over time rather than hard on/off
  if ( m_update_timer < kHoldTime ) { m_severity = std::min( m_severity + ( kFadeInRate * dt.asSeconds() ), 1.0f ); }
  else { m_severity = std::max( m_severity - ( kFadeOutRate * dt.asSeconds() ), 0.f ); }
  m_update_timer += dt;

  Sprites::UniformBuilder{}.set( "resolution", display_size ).set( "severity", m_severity ).apply( &get_shader() );
  set_position( { 0.f, 0.f } );

  m_player_last_health = Utils::Player::get_stats( reg ).health();
}

} // namespace Game::Sprites
