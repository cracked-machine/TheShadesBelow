#include <Components/Particle/SkeleBonesParticleSprite.hpp>

#include <Components/Random.hpp>
#include <Utils/Maths.hpp>
#include <array>
#include <cmath>
#include <random>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
void SkeleBonesParticle::emit()
{
  m_color_variation = Cmp::RandomInt( 0, 50 ).gen();

  // do_emit() already assigned this particle's own randomly-chosen lifetime before calling emit();
  // capture it so speed falloff can ease out relative to THIS particle's lifespan, not the sprite-wide max
  m_initial_lifetime = m_lifetime;

  // create random number generator
  static std::random_device rd;
  static std::mt19937 rng( rd() );

  // desync each particle's bounce so they don't all wobble in lockstep
  static std::uniform_real_distribution<float> bounce_phase_dist( 0.f, 2.f * std::numbers::pi_v<float> );

  // random orientation for the drawn polygon, independent of the direction it travels in
  static std::uniform_real_distribution<float> rotation_dist( 0.f, 360.f );

  // per-corner width jitter so the quad's long edges aren't parallel
  static std::uniform_real_distribution<float> width_jitter_dist( 0.5f, 1.5f );

  const sf::Angle angle = sf::degrees( m_angle_range( rng ) );
  const float speed = m_speed_range( rng );
  m_vertex.position = m_emitter_position;
  m_velocity = sf::Vector2f( speed, angle );
  m_bounce_phase = bounce_phase_dist( rng );
  m_rotation = sf::degrees( rotation_dist( rng ) );

  for ( auto &w : m_width_jitter )
    w = width_jitter_dist( rng );
};
} // namespace detail

SkeleBonesParticleSprite::SkeleBonesParticleSprite( size_t count )
    : SpriteBase( count ) {};

void SkeleBonesParticleSprite::simulate( sf::Time dt )
{
  for ( auto &p : m_particles_list )
  {
    // update the particle lifetime
    p.m_lifetime -= dt;

    // if the particle is dead, respawn it
    if ( p.m_lifetime <= sf::Time::Zero and not try_emit( p ) ) continue;

    // quad the lifetime ratio so speed drops off sharply early on and eases out near zero,
    // rather than bleeding off at a constant rate. Ease out against this particle's OWN initial
    // lifetime, not the sprite-wide max, otherwise short-lived particles start already decelerated
    const float initial_lifetime = p.m_initial_lifetime.asSeconds();
    const float lifetime_ratio = initial_lifetime > 0.f ? p.m_lifetime.asSeconds() / initial_lifetime : 0.f;
    const float speed_falloff = lifetime_ratio * lifetime_ratio * lifetime_ratio * lifetime_ratio;

    // update the position of the corresponding vertex
    p.m_vertex.position += p.m_velocity * speed_falloff * dt.asSeconds();

    // bounce: a sine wave on the Y-axis whose frequency ramps up towards the end of the
    // particle's life (like a dropped object's bounces getting quicker as it settles), while
    // its amplitude fades out with the same lifetime ratio so the bounce dies down with it.
    // Frequency is integrated into a running phase rather than evaluated at elapsed time
    // directly, otherwise the ramp would produce a discontinuous jump in the wave each frame.
    constexpr float kBounceFreqStart = 1.5f;
    constexpr float kBounceFreqEnd = 5.f;
    constexpr float kBounceAmplitude = 20.f;

    // Finetune the curve until the particles don't bounce when they come to a stop
    const float bounce_amplitude_falloff = std::pow( lifetime_ratio, 2.5f );

    const float bounce_freq = kBounceFreqStart + ( ( kBounceFreqEnd - kBounceFreqStart ) * ( 1.f - lifetime_ratio ) );
    p.m_bounce_phase += 2.f * std::numbers::pi_v<float> * bounce_freq * dt.asSeconds();
    const float bounce_speed = kBounceAmplitude * bounce_amplitude_falloff * std::sin( p.m_bounce_phase );
    p.m_vertex.position.y += bounce_speed * dt.asSeconds();

    p.m_vertex.color.a = 255;
    p.m_vertex.color.r = 211 + p.m_color_variation;
    p.m_vertex.color.g = 211 + p.m_color_variation;
    p.m_vertex.color.b = 211 + p.m_color_variation;
  }
}

void SkeleBonesParticleSprite::draw( sf::RenderTarget &target, sf::RenderStates states ) const
{
  states.texture = nullptr;
  states.blendMode = sf::BlendAlpha;

  // project out just the vertices for drawing
  std::vector<sf::Vertex> verts;
  std::vector<sf::Vertex> outline_verts;
  verts.reserve( m_particles_list.size() * 6 );
  SPDLOG_DEBUG( "Drawing {} particles", m_particles.size() );

  constexpr sf::Color kOutlineColour = sf::Color( 0, 0, 0, 128 );
  constexpr float kOutlineThickness = 3.f;

  // twigs are long and thin rather than square
  constexpr float kWidthRatio = 0.3f;

  for ( const auto &p : m_particles_list )
  {
    const float half_length = p.m_size;
    const float half_width = p.m_size * kWidthRatio;

    // map world -> screen
    const auto pos = m_world_to_screen( p.m_vertex.position );

    const auto colour = p.m_vertex.color;

    // rotate the polygon about its own centre point at the fixed random angle chosen on emit()
    const sf::Angle angle = p.m_rotation;

    // each corner's width is jittered independently, so the two long edges aren't parallel
    const std::array<sf::Vector2f, 4> corners = {
        pos + sf::Vector2f( -half_length, -half_width * p.m_width_jitter[0] ).rotatedBy( angle ),
        pos + sf::Vector2f( half_length, -half_width * p.m_width_jitter[1] ).rotatedBy( angle ),
        pos + sf::Vector2f( half_length, half_width * p.m_width_jitter[2] ).rotatedBy( angle ),
        pos + sf::Vector2f( -half_length, half_width * p.m_width_jitter[3] ).rotatedBy( angle ),
    };

    verts.push_back( { corners[0], colour } );
    verts.push_back( { corners[1], colour } );
    verts.push_back( { corners[2], colour } );

    verts.push_back( { corners[0], colour } );
    verts.push_back( { corners[2], colour } );
    verts.push_back( { corners[3], colour } );

    // build the outline as a quad per edge (rather than a hairline sf::PrimitiveType::Lines segment)
    // so the outline can have adjustable thickness.
    for ( size_t i = 0; i < corners.size(); ++i )
    {
      const auto &v0 = corners[i];
      const auto &v1 = corners[( i + 1 ) % corners.size()];

      const sf::Vector2f edge = v1 - v0;
      const sf::Vector2f normal = Utils::Maths::normalized( { -edge.y, edge.x } ).value_or( sf::Vector2f( 0.f, 0.f ) );
      const sf::Vector2f offset = normal * ( kOutlineThickness * 0.5f );

      const auto o0 = v0 + offset;
      const auto o1 = v0 - offset;
      const auto o2 = v1 - offset;
      const auto o3 = v1 + offset;

      outline_verts.push_back( { o0, kOutlineColour } );
      outline_verts.push_back( { o1, kOutlineColour } );
      outline_verts.push_back( { o2, kOutlineColour } );

      outline_verts.push_back( { o0, kOutlineColour } );
      outline_verts.push_back( { o2, kOutlineColour } );
      outline_verts.push_back( { o3, kOutlineColour } );
    }
  }

  target.draw( verts.data(), verts.size(), sf::PrimitiveType::Triangles, states );
  target.draw( outline_verts.data(), outline_verts.size(), sf::PrimitiveType::Triangles, states );
}

} // namespace Game::Cmp::Particle