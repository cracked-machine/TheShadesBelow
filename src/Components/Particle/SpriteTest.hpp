#ifndef SRC_COMPONENTS_PARTICLE_SPRITETEST_HPP__
#define SRC_COMPONENTS_PARTICLE_SPRITETEST_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/BlendMode.hpp>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual particle for the generic test sprite, drawn as a fixed-colour dot flying outward at a random angle/speed.
struct ParticleTest : public Cmp::Particle::ParticleBase
{

private:
  //! @brief Rolls a new random angle/speed and sets the resulting velocity on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Generic test particle sprite, used for debugging/prototyping particle effects (see Factory::Particle::add_test()).
class SpriteTest : public SpriteBase<detail::ParticleTest>
{
public:
  //! @brief Construct a new Sprite Test object
  //! @param count Number of particles in this sprite
  SpriteTest( size_t count );

  //! @brief Advances the simulation by one frame: moves each particle and fixes its colour to opaque magenta.
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_SPRITETEST_HPP__
