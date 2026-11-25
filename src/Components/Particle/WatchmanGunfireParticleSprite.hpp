#ifndef SRC_COMPONENTS_PARTICLE_WatchmanGunfireParticleSprite_HPP__
#define SRC_COMPONENTS_PARTICLE_WatchmanGunfireParticleSprite_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/BlendMode.hpp>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual particle for a watchman's gunfire burst, fired at a fixed speed/lifetime toward the player.
struct WatchmanGunfireParticle : public Cmp::Particle::ParticleBase
{

private:
  //! @brief Launches the particle on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Particle sprite for a gunfire burst from a watchman, aimed at the player's current position with a small random spread.
class WatchmanGunfireParticleSprite : public SpriteBase<detail::WatchmanGunfireParticle>
{
public:
  //! @brief Radius (world px) of the drawn shot, excluding its outline. Also pads NPC hitboxes so a shot that visibly
  //! overlaps an NPC hits it.
  static constexpr float kShotRadius = 6.f;

  //! @brief Construct a new Watchman Gunfire Particle Sprite object
  //! @param count Number of particles in this sprite
  WatchmanGunfireParticleSprite( size_t count );

  //! @brief Advances the gunfire simulation by one frame.
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;
  //! @brief Draws each particle.
  //! @param target Render target to draw to.
  //! @param states Render states (transform/blend mode) to draw with.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_WatchmanGunfireParticleSprite_HPP__
