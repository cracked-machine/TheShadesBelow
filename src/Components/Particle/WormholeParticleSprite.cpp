#include <Components/Particle/WormholeParticleSprite.hpp>

#include <random>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
void WormholeParticle::emit()
{
  // create random number generator
  static std::random_device rd;
  static std::mt19937 rng( rd() );
  static std::uniform_real_distribution<float> phase_dist( 0.f, 2.f * std::numbers::pi_v<float> );
  static std::uniform_real_distribution<float> freq_dist( 2.f, 5.f );

  // spawn on a circle around the emitter, rather than at its centre
  constexpr float kSpawnRadius = 16.f;

  const sf::Angle angle = sf::degrees( m_angle_range( rng ) );
  const float speed = m_speed_range( rng );
  m_vertex.position = m_emitter_position + sf::Vector2f( kSpawnRadius, angle );
  m_velocity = sf::Vector2f( speed, angle );

  // perpendicular to the outward direction, used to bend the trajectory into a sine wave
  m_perpendicular = sf::Vector2f( 1.f, angle + sf::degrees( 90.f ) );
  m_wave_time = 0.f;
  m_phase = phase_dist( rng );
  m_frequency = freq_dist( rng );
};
} // namespace detail

WormholeParticleSprite::WormholeParticleSprite( size_t count )
    : SpriteBase( count ) {};

void WormholeParticleSprite::simulate( sf::Time dt )
{
  constexpr float kWaveAmplitude = 20.f;

  for ( auto &p : m_particles_list )
  {
    // update the particle lifetime
    p.m_lifetime -= dt;
    p.m_wave_time += dt.asSeconds();

    // if the particle is dead, respawn it
    if ( p.m_lifetime <= sf::Time::Zero and not try_emit( p ) ) continue;

    // sideways oscillation, perpendicular to the outward direction
    const float wave_speed =
        kWaveAmplitude *
        std::sin( ( 2.f * std::numbers::pi_v<float> * p.m_frequency * p.m_wave_time ) + p.m_phase );

    // update the position of the corresponding vertex
    p.m_vertex.position += ( p.m_velocity + p.m_perpendicular * wave_speed ) * dt.asSeconds();

    p.m_vertex.color.a = 192;
    p.m_vertex.color.r = 192;
    p.m_vertex.color.g = 192;
    p.m_vertex.color.b = 192;
  }
}

//! @brief Allows this sprite to be passed into RenderWindow.draw()
//! @param target
//! @param states
void WormholeParticleSprite::draw( sf::RenderTarget &target, sf::RenderStates states ) const
{
  states.texture = nullptr;
  states.blendMode = sf::BlendAlpha;

  // project out just the vertices for drawing
  std::vector<sf::Vertex> verts;
  verts.reserve( m_particles_list.size() );
  SPDLOG_DEBUG( "Drawing {} particles", m_particles.size() );

  constexpr float kSize = 2.f;
  constexpr int kSides = 3;
  for ( const auto &p : m_particles_list )
  {

    // map world -> screen
    const auto pos = m_world_to_screen( p.m_vertex.position );

    const auto colour = p.m_vertex.color;

    // unit circle divided into wedges
    constexpr float kAngleStep = 2.f * std::numbers::pi_v<float> / static_cast<float>( kSides );

    for ( int i = 0; i < kSides; ++i )
    {
      // the two outside facing (polar) angles for the current wedge
      const float a0 = kAngleStep * static_cast<float>( i );
      const float a1 = kAngleStep * static_cast<float>( i + 1 );

      // the centre position is always the middle of the polygon
      verts.push_back( { pos, colour } );
      // get the vertex positions by converting the polar coords to cartesian coords (euler's formula).
      verts.push_back( { pos + sf::Vector2f( kSize, sf::radians( a0 ) ), colour } );
      verts.push_back( { pos + sf::Vector2f( kSize, sf::radians( a1 ) ), colour } );
    }
  }

  target.draw( verts.data(), verts.size(), sf::PrimitiveType::Triangles, states );
}

} // namespace Game::Cmp::Particle