#ifndef SRC_COMPONENTS_PARTICLE_WORMHOLEPARTICLESPRITE_HPP__
#define SRC_COMPONENTS_PARTICLE_WORMHOLEPARTICLESPRITE_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/BlendMode.hpp>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual particle for the wormhole effect, swaying about its travel path.
struct WormholeParticle : public Cmp::Particle::ParticleBase
{
  //! @brief Elapsed time (seconds) since this particle's own wave motion last reset, independent of the other particles.
  float m_wave_time{ 0.f };
  //! @brief This particle's own randomly-assigned sway wave phase.
  float m_phase{ 0.f };
  //! @brief This particle's own randomly-assigned sway wave frequency.
  float m_frequency{ 0.5f };
  //! @brief Unit vector perpendicular to the particle's travel direction, used to offset it sideways for the sway motion.
  sf::Vector2f m_perpendicular{ 0.f, 0.f };

private:
  //! @brief Rolls a new phase/frequency and velocity/perpendicular on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Particle sprite for a wormhole visual effect.
class WormholeParticleSprite : public SpriteBase<detail::WormholeParticle>
{
public:
  //! @brief Construct a new Wormhole Particle Sprite object
  //! @param count Number of particles in this sprite
  WormholeParticleSprite( size_t count );

  //! @brief Advances the wormhole simulation by one frame.
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;
  //! @brief Draws each particle.
  //! @param target Render target to draw to.
  //! @param states Render states (transform/blend mode) to draw with.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_WORMHOLEPARTICLESPRITE_HPP__
