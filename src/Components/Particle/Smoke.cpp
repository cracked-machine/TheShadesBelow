#include <Components/Particle/Smoke.hpp>

#include <SFML/System/Vector2.hpp>
#include <numbers>
#include <random>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
void SmokeParticle::emit()
{
  static std::random_device rd;
  static std::mt19937 rng( rd() );

  m_wave_time = 0.f;

  m_phase = m_phase_range( rng );
  m_frequency = m_freq_range( rng );

  m_velocity = sf::Vector2f( 0.f, -m_speed_range( rng ) );
};
} // namespace detail

Smoke::Smoke( size_t count )
    : SpriteBase( count ) {};

void Smoke::simulate( sf::Time dt )
{
  m_elapsed += dt.asSeconds();

  const float amplitude = 1.f;
  const sf::Color smoke_color{ 0, 0, 0 };
  constexpr float k_smoke_density = 0.5f; // 10% of particles visible in smoke phase

  static std::mt19937 rng( std::random_device{}() );
  static std::uniform_real_distribution<float> density_dist( 0.f, 1.f );

  for ( auto &p : m_particles_list )
  {
    // p.m_emitter.y -= m_rise_speed * dt.asSeconds(); // move emitter upward over time
    p.m_lifetime -= dt;
    p.m_wave_time += dt.asSeconds();
    const float ratio = p.m_lifetime.asSeconds() / m_max_lifetime.asSeconds();
    if ( p.m_lifetime <= sf::Time::Zero and not try_emit( p ) ) continue;

    const float wave_x = ( ratio > 0.8f ) ? amplitude * std::sin( ( 2.f * std::numbers::pi_v<float> * p.m_frequency * p.m_wave_time ) + p.m_phase )
                                          : 0.f;

    p.m_vertex.position += sf::Vector2f{ wave_x, p.m_velocity.y } * dt.asSeconds();

    const bool visible = density_dist( rng ) < k_smoke_density;
    p.m_vertex.color = smoke_color;
    p.m_vertex.color.a = visible ? static_cast<std::uint8_t>( ratio * 255 ) : 0;
  }
}

void Smoke::draw( sf::RenderTarget &target, sf::RenderStates states ) const
{
  states.texture = nullptr;
  states.blendMode = sf::BlendAlpha;

  // project out just the vertices for drawing
  std::vector<sf::Vertex> verts;
  verts.reserve( m_particles_list.size() );
  SPDLOG_DEBUG( "Drawing {} particles", m_particles.size() );

  constexpr float kSize = 2.f;
  constexpr int kSides = 4;
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