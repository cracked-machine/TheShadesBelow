#include <Components/Particle/WatchmanGunfireParticleSprite.hpp>

#include <Utils/Player.hpp>
#include <random>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
void WatchmanGunfireParticle::emit()
{
  // create random number generator
  static std::random_device rd;
  static std::mt19937 rng( rd() );

  const sf::Angle angle = sf::degrees( m_angle_range( rng ) );
  const float speed = m_speed_range( rng );

  m_velocity = sf::Vector2f( speed, angle );
};
} // namespace detail

WatchmanGunfireParticleSprite::WatchmanGunfireParticleSprite( size_t count )
    : SpriteBase( count ) {};

void WatchmanGunfireParticleSprite::simulate( sf::Time dt )
{
  for ( auto &p : m_particles_list )
  {
    // update the particle lifetime
    p.m_lifetime -= dt;

    // if the particle is dead, respawn it
    if ( p.m_lifetime <= sf::Time::Zero and not try_emit( p ) ) continue;

    // update the position of the corresponding vertex
    p.m_vertex.position += p.m_velocity * dt.asSeconds();

    p.m_vertex.color.a = 255;
    p.m_vertex.color.r = 200;
    p.m_vertex.color.g = 200;
    p.m_vertex.color.b = 200;
  }
}

void WatchmanGunfireParticleSprite::draw( sf::RenderTarget &target, sf::RenderStates states ) const
{
  states.texture = nullptr;
  states.blendMode = sf::BlendAlpha;

  // project out just the vertices for drawing
  std::vector<sf::Vertex> verts;
  verts.reserve( m_particles_list.size() * 2 );
  SPDLOG_DEBUG( "Drawing {} particles", m_particles.size() );

  constexpr float kSize = 6.f;
  constexpr float kOutlineThickness = 2.f;
  constexpr int kSides = 8;

  // unit circle divided into wedges
  constexpr float kAngleStep = 2.f * std::numbers::pi_v<float> / static_cast<float>( kSides );

  auto push_polygon = [&]( sf::Vector2f pos, float radius, sf::Color colour )
  {
    for ( int i = 0; i < kSides; ++i )
    {
      // the two outside facing (polar) angles for the current wedge
      const float a0 = kAngleStep * static_cast<float>( i );
      const float a1 = kAngleStep * static_cast<float>( i + 1 );

      // the centre position is always the middle of the polygon
      verts.push_back( { pos, colour } );
      // get the vertex positions by converting the polar coords to cartesian coords (euler's formula).
      verts.push_back( { pos + sf::Vector2f( radius, sf::radians( a0 ) ), colour } );
      verts.push_back( { pos + sf::Vector2f( radius, sf::radians( a1 ) ), colour } );
    }
  };

  for ( const auto &p : m_particles_list )
  {
    // map world -> screen
    const auto pos = m_world_to_screen( p.m_vertex.position );

    const auto colour = p.m_vertex.color;

    // black outline drawn first, slightly larger than the fill so it peeks out from behind it
    push_polygon( pos, kSize + kOutlineThickness, sf::Color( 0, 0, 0, colour.a ) );
    push_polygon( pos, kSize, colour );
  }

  target.draw( verts.data(), verts.size(), sf::PrimitiveType::Triangles, states );
}

} // namespace Game::Cmp::Particle