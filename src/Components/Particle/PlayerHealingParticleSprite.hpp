#ifndef SRC_COMPONENTS_PARTICLE_PLAYERHEALINGPARTICLESPRITE_HPP__
#define SRC_COMPONENTS_PARTICLE_PLAYERHEALINGPARTICLESPRITE_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/BlendMode.hpp>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual particle for the visual effect played when the player is healed.
struct PlayerHealingParticle : public Cmp::Particle::ParticleBase
{

private:
  //! @brief Launches the particle on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Particle sprite for the visual effect played when the player is healed.
class PlayerHealingParticleSprite : public SpriteBase<detail::PlayerHealingParticle>
{
public:
  //! @brief Construct a new Player Healing Particle Sprite object
  //! @param count Number of particles in this sprite
  PlayerHealingParticleSprite( size_t count );

  //! @brief Advances the healing effect simulation by one frame.
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;
  //! @brief Draws each particle.
  //! @param target Render target to draw to.
  //! @param states Render states (transform/blend mode) to draw with.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_PLAYERHEALINGPARTICLESPRITE_HPP__
