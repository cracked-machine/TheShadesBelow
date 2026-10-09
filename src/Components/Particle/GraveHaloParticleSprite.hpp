#ifndef SRC_COMPONENTS_PARTICLE_GRAVEHALOPARTICLESPRITE_HPP__
#define SRC_COMPONENTS_PARTICLE_GRAVEHALOPARTICLESPRITE_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/BlendMode.hpp>
#include <Systems/ParticleSystem.hpp>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual particle for the rune particle effect.
struct GraveHaloParticle : public Cmp::Particle::ParticleBase
{

private:
  //! @brief Launches the particle on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Particle sprite for a rune visual effect.
class GraveHaloParticleSprite : public SpriteBase<detail::GraveHaloParticle>
{
public:
  //! @brief Tag of the player's floating music-note particles, followed every frame to the player's feet.
  static constexpr std::string_view kTag = "graveyard.grave.particle.halo";

  //! @brief Construct a new Rune Particle Sprite object
  //! @param count Number of particles in this sprite
  GraveHaloParticleSprite( size_t count );

  //! @brief Advances the rune effect simulation by one frame.
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;
  //! @brief Draws each particle.
  //! @param target Render target to draw to.
  //! @param states Render states (transform/blend mode) to draw with.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_GRAVEHALOPARTICLESPRITE_HPP__
