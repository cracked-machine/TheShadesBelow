#include <Components/Particle/EmberParticleSprite.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
void EmberParticle::emit()
{
  // create random number generator
  static std::random_device rd;
  static std::mt19937 rng( rd() );

  const sf::Angle angle = sf::degrees( m_angle_range( rng ) );
  const float speed = m_speed_range( rng );

  m_velocity = sf::Vector2f( speed, angle );

  // time for gravity to cancel the (negative, i.e. upwards) launch velocity, and then return it to the launch height
  m_flight_time = 2.f * -m_velocity.y / EmberParticleSprite::kGravity;
  m_flight_elapsed = 0.f;
};
} // namespace detail

const float EmberParticleSprite::kGravity = 280.f;

EmberParticleSprite::EmberParticleSprite( size_t count )
    : SpriteBase( count ) {};

void EmberParticleSprite::simulate( sf::Time dt )
{
  static std::mt19937 rng( std::random_device{}() );
  static std::uniform_int_distribution<int> flicker_dist( 170, 255 );

  const sf::Color kHotColour{ 255, 240, 170 };
  const sf::Color kWarmColour{ 255, 130, 20 };
  const sf::Color kCoolColour{ 140, 20, 0 };

  auto lerp_colour = []( sf::Color from, sf::Color to, float t ) -> sf::Color
  {
    auto lerp_channel = [t]( std::uint8_t a, std::uint8_t b )
    { return static_cast<std::uint8_t>( std::lerp( static_cast<float>( a ), static_cast<float>( b ), t ) ); };
    return { lerp_channel( from.r, to.r ), lerp_channel( from.g, to.g ), lerp_channel( from.b, to.b ) };
  };

  for ( auto &p : m_particles_list )
  {
    // update the particle lifetime
    p.m_lifetime -= dt;

    // if the particle is dead, respawn it
    if ( p.m_lifetime <= sf::Time::Zero and not try_emit( p ) ) continue;

    // a stopped particle that has landed/expired stays hidden until it is pruned
    if ( not p.m_particle_active and p.m_lifetime <= sf::Time::Zero )
    {
      p.m_vertex.color.a = 0;
      continue;
    }

    // y grows downwards: the particle is launched with a negative y velocity, so it sits above the
    // emitter (smaller y) for the whole flight, while gravity pulls the y velocity back to positive.
    p.m_velocity.y += kGravity * dt.asSeconds();
    p.m_vertex.position += p.m_velocity * dt.asSeconds();

    // cool from white-hot to orange over the first half of the flight, then on down to dark red
    p.m_flight_elapsed += dt.asSeconds();
    const float progress = p.m_flight_time > 0.f ? std::clamp( p.m_flight_elapsed / p.m_flight_time, 0.f, 1.f ) : 1.f;
    p.m_vertex.color = progress < 0.5f ? lerp_colour( kHotColour, kWarmColour, progress * 2.f )
                                       : lerp_colour( kWarmColour, kCoolColour, ( progress - 0.5f ) * 2.f );
    // embers glow unevenly
    p.m_vertex.color.a = static_cast<std::uint8_t>( flicker_dist( rng ) );

    // falling, and back down at the height it was emitted from: kill it.
    // The velocity check stops this from firing on the emit frame, where the particle is still at the emitter.
    if ( p.m_velocity.y > 0.f and p.m_vertex.position.y >= p.m_emitter_position.y )
    {
      p.m_lifetime = sf::Time::Zero;
      p.m_vertex.color.a = 0;
    }
  }
}

void EmberParticleSprite::draw( sf::RenderTarget &target, sf::RenderStates states ) const
{
  states.texture = nullptr;
  states.blendMode = sf::BlendAlpha;

  // project out just the vertices for drawing
  std::vector<sf::Vertex> verts;

  // a diamond: 4 wedges, with a vertex pointing along each axis
  constexpr int kSides = 4;
  constexpr int kLayerCount = 2;
  verts.reserve( m_particles_list.size() * kLayerCount * kSides * 3 );

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

  constexpr float kGlowScale = 2.f;

  for ( const auto &p : m_particles_list )
  {
    const auto alpha = p.m_vertex.color.a;
    if ( alpha == 0 ) continue;

    // map world -> screen
    const auto pos = m_world_to_screen( p.m_vertex.position );
    // p.m_size is a world-unit size; scale it by the camera zoom (see SpriteBase::set_view_transform())
    const float radius = p.m_size * m_world_to_screen_scale;

    // faint red glow drawn first, larger than the ember so it peeks out from behind it
    push_polygon( pos, radius * kGlowScale, sf::Color( 200, 40, 0, static_cast<std::uint8_t>( alpha / 3 ) ) );
    push_polygon( pos, radius, p.m_vertex.color );
  }

  target.draw( verts.data(), verts.size(), sf::PrimitiveType::Triangles, states );
}

} // namespace Game::Cmp::Particle
