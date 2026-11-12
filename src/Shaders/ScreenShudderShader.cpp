#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

#include <Components/Persistent/DisplayResolution.hpp>
#include <Shaders/UniformBuilder.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>

#include <Shaders/ScreenShudderShader.hpp>

namespace Game::Sprites
{

namespace
{
constexpr float kTwoPi = 2.f * std::numbers::pi_v<float>;
// Fraction of the duration spent ramping up: the door breaking loose
constexpr float kAttackFraction = 0.1f;
// Fraction of the duration spent easing out: the door settling
constexpr float kReleaseFraction = 0.3f;
// Sub-rumble and sideways judder rates, relative to the main rumble frequency. Not whole multiples, so the
// combined motion never settles into an obvious repeating pattern.
constexpr float kSubRumbleRatio = 0.37f;
constexpr float kJudderRatio = 1.3f;
// How much each component contributes to the shake (vertical components dominate: it's a heavy, ground-borne rumble)
constexpr float kRumbleWeight = 0.55f;
constexpr float kSubRumbleWeight = 0.35f;
constexpr float kJudderWeight = 0.3f;
constexpr float kJoltWeight = 0.4f;
// How often the irregular "grinding stone" jolt picks a new target
constexpr sf::Time kJoltInterval = sf::milliseconds( 60 );

// Cheap integer hash to a float in [-1, 1]
float hash_signed( std::uint32_t n )
{
  n = ( n ^ 61u ) ^ ( n >> 16 );
  n *= 9u;
  n ^= n >> 4;
  n *= 0x27d4eb2du;
  n ^= n >> 15;
  return ( static_cast<float>( n & 0xFFFFu ) / 32767.5f ) - 1.f;
}

float smoothstep( float edge0, float edge1, float x )
{
  const float t = std::clamp( ( x - edge0 ) / ( edge1 - edge0 ), 0.f, 1.f );
  return t * t * ( 3.f - ( 2.f * t ) );
}

void advance_phase( float &phase, float frequency, float dt_secs ) { phase = std::fmod( phase + ( kTwoPi * frequency * dt_secs ), kTwoPi ); }
} // namespace

void ScreenShudderShader::trigger()
{
  m_elapsed = sf::Time::Zero;
  active( m_duration > sf::Time::Zero );
}

void ScreenShudderShader::stop()
{
  m_elapsed = sf::Time::Zero;
  active( false );
}

void ScreenShudderShader::update( entt::registry &reg, sf::Time dt )
{
  if ( not active() ) return;

  m_elapsed += dt;
  if ( m_elapsed >= m_duration )
  {
    stop();
    return;
  }

  const float t = m_elapsed / m_duration;
  // quick attack, sustained grind, smooth settle
  const float envelope = smoothstep( 0.f, kAttackFraction, t ) * ( 1.f - smoothstep( 1.f - kReleaseFraction, 1.f, t ) );

  // accumulate the phases rather than deriving them from elapsed time, so changing the frequency mid-shudder
  // doesn't make the frame jump
  const float dt_secs = dt.asSeconds();
  advance_phase( m_rumble_phase, m_frequency, dt_secs );
  advance_phase( m_sub_phase, m_frequency * kSubRumbleRatio, dt_secs );
  advance_phase( m_judder_phase, m_frequency * kJudderRatio, dt_secs );

  // irregular jolt: blend between random targets re-rolled every kJoltInterval
  if ( ( m_jolt_timer += dt ) >= kJoltInterval )
  {
    m_jolt_timer = sf::Time::Zero;
    m_jolt_from = m_jolt_to;
    ++m_jolt_seed;
    m_jolt_to = { hash_signed( m_jolt_seed * 2u ), hash_signed( ( m_jolt_seed * 2u ) + 1u ) };
  }
  const float jolt_t = smoothstep( 0.f, 1.f, m_jolt_timer / kJoltInterval );
  const sf::Vector2f jolt = m_jolt_from + ( ( m_jolt_to - m_jolt_from ) * jolt_t );

  const sf::Vector2f shake_px = sf::Vector2f{ ( kJudderWeight * std::sin( m_judder_phase ) ) + ( kJoltWeight * 0.5f * jolt.x ),
                                              ( kRumbleWeight * std::sin( m_rumble_phase ) ) + ( kSubRumbleWeight * std::sin( m_sub_phase ) ) +
                                                  ( kJoltWeight * jolt.y ) } *
                                ( m_amplitude * envelope );

  // world pixels -> screen UV, y flipped to match gl_FragCoord
  const sf::Vector2f view_size = Sys::RenderSystem::get_world_view().getSize();
  const sf::Vector2f offset_uv{ shake_px.x / view_size.x, -shake_px.y / view_size.y };

  auto display_size = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg ) );
  Sprites::UniformBuilder{}.set( "resolution", display_size ).set( "offset", offset_uv ).set( "strength", envelope ).apply( &get_shader() );

  set_position( { 0.f, 0.f } );
}

} // namespace Game::Sprites
