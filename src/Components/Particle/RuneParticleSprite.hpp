#ifndef SRC_COMPONENTS_PARTICLE_RUNEPARTICLESPRITE_HPP__
#define SRC_COMPONENTS_PARTICLE_RUNEPARTICLESPRITE_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/BlendMode.hpp>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual particle for the rune particle effect.
struct RuneParticle : public Cmp::Particle::ParticleBase
{

private:
  //! @brief Launches the particle on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Particle sprite for a rune visual effect.
class RuneParticleSprite : public SpriteBase<detail::RuneParticle>
{
public:
  //! @brief Construct a new Rune Particle Sprite object
  //! @param count Number of particles in this sprite
  RuneParticleSprite( size_t count );

  //! @brief Advances the rune effect simulation by one frame.
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;
  //! @brief Draws each particle.
  //! @param target Render target to draw to.
  //! @param states Render states (transform/blend mode) to draw with.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_RUNEPARTICLESPRITE_HPP__
