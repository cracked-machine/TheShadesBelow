#include <Components/Particle/LavaFireballParticleSprite.hpp>

#include <Utils/Maths.hpp>
#include <algorithm>
#include <numbers>
#include <random>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
void LavaFireballParticle::emit()
{
  // create random number generator
  static std::random_device rd;
  static std::mt19937 rng( rd() );

  const sf::Angle angle = sf::degrees( m_angle_range( rng ) );
  const float speed = m_speed_range( rng );

  m_velocity = sf::Vector2f( speed, angle );

  // roll a new outline on the first simulate() step
  m_flicker_countdown = sf::Time::Zero;
};
} // namespace detail

const float LavaFireballParticleSprite::kGravity = 280.f;

LavaFireballParticleSprite::LavaFireballParticleSprite( size_t count )
    : SpriteBase( count ) {};

void LavaFireballParticleSprite::simulate( sf::Time dt )
{
  static std::mt19937 rng( std::random_device{}() );
  static std::uniform_real_distribution<float> jitter_dist( -1.f, 1.f );
  const sf::Time kFlickerInterval = sf::milliseconds( 60 );

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

    p.m_vertex.color = sf::Color( 255, 110, 20 );

    // re-roll the ragged outline every so often, rather than every frame, so it flickers instead of buzzing
    p.m_flicker_countdown -= dt;
    if ( p.m_flicker_countdown <= sf::Time::Zero )
    {
      p.m_flicker_countdown = kFlickerInterval;
      for ( auto &jitter : p.m_vertex_jitter )
        jitter = jitter_dist( rng );
    }

    // falling, and back down at the height it was emitted from: kill it.
    // The velocity check stops this from firing on the emit frame, where the particle is still at the emitter.
    if ( p.m_velocity.y > 0.f and p.m_vertex.position.y >= p.m_emitter_position.y )
    {
      p.m_lifetime = sf::Time::Zero;
      p.m_vertex.color.a = 0;
    }
  }
}

void LavaFireballParticleSprite::draw( sf::RenderTarget &target, sf::RenderStates states ) const
{
  states.texture = nullptr;
  states.blendMode = sf::BlendAlpha;

  // project out just the vertices for drawing
  std::vector<sf::Vertex> verts;

  constexpr int kVertexCount = detail::LavaFireballParticle::kVertexCount;
  constexpr int kLayerCount = 3;
  verts.reserve( m_particles_list.size() * kLayerCount * kVertexCount * 3 );

  // unit circle divided into wedges
  constexpr float kAngleStep = 2.f * std::numbers::pi_v<float> / static_cast<float>( kVertexCount );

  //! @brief One flame layer of the fireball: a ragged polygon, stretched out behind the direction of travel.
  struct Layer
  {
    //! @brief Radius of the layer relative to the particle size.
    float scale;
    //! @brief How far the per-vertex jitter pushes the outline in/out, as a fraction of the radius.
    float raggedness;
    //! @brief How far the vertices facing away from the direction of travel are stretched into flame tongues.
    float tail_stretch;
    //! @brief Offset into the particle's jitter table, so the layers don't share the same outline.
    int jitter_offset;
  };

  // Sizes must respect draw order: glow > body > core
  constexpr Layer kGlowLayer{ .scale = 1.f, .raggedness = 0.25f, .tail_stretch = 1.2f, .jitter_offset = 0 };
  constexpr Layer kBodyLayer{ .scale = 0.8f, .raggedness = 0.25f, .tail_stretch = 1.0f, .jitter_offset = 5 };
  constexpr Layer kCoreLayer{ .scale = 0.5f, .raggedness = 0.2f, .tail_stretch = 0.8f, .jitter_offset = 9 };

  for ( const auto &p : m_particles_list )
  {
    const auto alpha = p.m_vertex.color.a;
    if ( alpha == 0 ) continue;

    // map world -> screen
    const auto pos = m_world_to_screen( p.m_vertex.position );
    // p.m_size is a world-unit size; scale it by the camera zoom (see SpriteBase::set_view_transform())
    const float radius = p.m_size * m_world_to_screen_scale;

    // flames stream out behind the fireball, i.e. opposite to its direction of travel
    const sf::Vector2f tail_direction = -Utils::Maths::normalized( p.m_velocity ).value_or( sf::Vector2f( 0.f, -1.f ) );

    auto push_layer = [&]( const Layer &layer, sf::Color colour )
    {
      auto outline_vertex = [&]( int i ) -> sf::Vector2f
      {
        const sf::Vector2f direction( 1.f, sf::radians( kAngleStep * static_cast<float>( i ) ) );
        const float jitter = p.m_vertex_jitter[static_cast<size_t>( ( i + layer.jitter_offset ) % kVertexCount )];

        // 0 for vertices on the leading half, rising to 1 for the vertex pointing straight back
        const float tail_facing = std::max( 0.f, direction.dot( tail_direction ) );
        // the jitter also varies the length of each flame tongue, so the tail is uneven
        const float stretch = 1.f + ( tail_facing * tail_facing * layer.tail_stretch * ( 1.f + ( jitter * 0.5f ) ) );

        return pos + ( direction * radius * layer.scale * ( 1.f + ( jitter * layer.raggedness ) ) * stretch );
      };

      for ( int i = 0; i < kVertexCount; ++i )
      {
        // the centre position is always the middle of the polygon
        verts.push_back( { pos, colour } );
        verts.push_back( { outline_vertex( i ), colour } );
        verts.push_back( { outline_vertex( ( i + 1 ) % kVertexCount ), colour } );
      }
    };

    // dark glow drawn first, larger than the body so it peeks out from behind it, then the hot core on top
    push_layer( kGlowLayer, sf::Color( 200, 30, 0, static_cast<std::uint8_t>( alpha / 2 ) ) );
    push_layer( kBodyLayer, p.m_vertex.color );
    push_layer( kCoreLayer, sf::Color( 255, 230, 120, alpha ) );
  }

  target.draw( verts.data(), verts.size(), sf::PrimitiveType::Triangles, states );
}

} // namespace Game::Cmp::Particle
