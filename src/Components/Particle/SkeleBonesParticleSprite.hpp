#ifndef SRC_COMPONENTS_PARTICLE_SKELEBONESPARTICLESPRITE_HPP__
#define SRC_COMPONENTS_PARTICLE_SKELEBONESPARTICLESPRITE_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/BlendMode.hpp>

#include <array>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual bone-debris particle for a skeleton killed by the axe, drawn as a jittered elongated quad.
struct SkeleBonesParticle : public Cmp::Particle::ParticleBase
{
  //! @brief Add some variation to colour of each particle
  int m_color_variation = 0;

  //! @brief This particle's own randomly-assigned lifetime at spawn, captured so speed falloff can
  //!        ease out relative to it rather than the sprite-wide max lifetime
  sf::Time m_initial_lifetime;

  //! @brief Running phase (radians) for the vertical bounce oscillation. Advanced each frame by the
  //!        current bounce frequency rather than derived from elapsed time directly, so the frequency
  //!        can ramp up smoothly over the particle's lifetime without a discontinuity in the wave
  float m_bounce_phase = 0.f;

  //! @brief Fixed rotation of the drawn polygon about its own centre, randomized once on emit()
  sf::Angle m_rotation;

  //! @brief Per-corner width multipliers (near-start, far-start, far-end, near-end), randomized once on
  //!        emit() so the drawn quad's long edges aren't parallel — width varies irregularly end-to-end
  std::array<float, 4> m_width_jitter{ 1.f, 1.f, 1.f, 1.f };

private:
  //! @brief Rolls new rotation/width jitter and launches the particle on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Particle sprite for the bone debris effect played when a skeleton is killed by the axe.
class SkeleBonesParticleSprite : public SpriteBase<detail::SkeleBonesParticle>
{
public:
  //! @brief Construct a new Skele Bones Particle Sprite object
  //! @param count Number of particles in this sprite
  SkeleBonesParticleSprite( size_t count );

  //! @brief Advances the bone debris simulation by one frame (bounce, speed falloff).
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;
  //! @brief Draws each particle as a jittered elongated quad rather than the base class's dot.
  //! @param target Render target to draw to.
  //! @param states Render states (transform/blend mode) to draw with.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_SKELEBONESPARTICLESPRITE_HPP__
