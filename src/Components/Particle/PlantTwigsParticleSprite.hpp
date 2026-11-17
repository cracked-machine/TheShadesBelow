#ifndef SRC_COMPONENTS_PARTICLE_PLANTTWIGSPARTICLESPRITE_HPP__
#define SRC_COMPONENTS_PARTICLE_PLANTTWIGSPARTICLESPRITE_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/BlendMode.hpp>
#include <Systems/ParticleSystem.hpp>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual twig-debris particle for a plant destroyed by the shovel.
struct PlantTwigsParticle : public Cmp::Particle::ParticleBase
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

private:
  //! @brief Launches the particle on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Particle sprite for the twig debris effect played when a plant is destroyed by the shovel.
class PlantTwigsParticleSprite : public SpriteBase<detail::PlantTwigsParticle>
{
public:
  //! @brief Construct a new Plant Twigs Particle Sprite object
  //! @param count Number of particles in this sprite
  PlantTwigsParticleSprite( size_t count );

  //! @brief Advances the twig debris simulation by one frame (bounce, speed falloff).
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;
  //! @brief Draws each particle as its own shape rather than the base class's dot.
  //! @param target Render target to draw to.
  //! @param states Render states (transform/blend mode) to draw with.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_PLANTTWIGSPARTICLESPRITE_HPP__
