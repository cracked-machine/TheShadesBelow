#include <Components/Particle/SpriteTest.hpp>

#include <random>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
void ParticleTest::emit()
{
  // create random number generator
  static std::random_device rd;
  static std::mt19937 rng( rd() );

  const sf::Angle angle = sf::degrees( m_angle_range( rng ) );
  const float speed = m_speed_range( rng );
  m_velocity = sf::Vector2f( speed, angle );
};
} // namespace detail

SpriteTest::SpriteTest( size_t count )
    : SpriteBase( count ) {};

void SpriteTest::simulate( sf::Time dt )
{
  for ( auto &p : m_particles_list )
  {
    // update the particle lifetime
    p.m_lifetime -= dt;

    // if the particle is dead, respawn it
    if ( p.m_lifetime <= sf::Time::Zero and not try_emit( p ) ) continue;

    // update the position of the corresponding vertex
    p.m_vertex.position += p.m_velocity * dt.asSeconds();

    p.m_vertex.color.a = 128;
    p.m_vertex.color.r = 255;
    p.m_vertex.color.g = 0;
    p.m_vertex.color.b = 255;
  }
}

} // namespace Game::Cmp::Particle