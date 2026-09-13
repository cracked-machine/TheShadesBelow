#ifndef SRC_COMPONENTS_PARTICLE_FLAME_HPP__
#define SRC_COMPONENTS_PARTICLE_FLAME_HPP__

#include <Components/Particle/SpriteBase.hpp>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual particle for the flame effect (candle/altar fire). Rises and sways, fading
//!        from white through the sprite's target colour before guttering out as smoke.
struct FlameParticle : public Cmp::Particle::ParticleBase
{
  //! @brief Elapsed time (seconds) since this particle's own wave motion last reset, independent of the other particles.
  float m_wave_time{ 0.f };
  //! @brief This particle's own randomly-assigned sway wave phase.
  float m_phase{ 0.f };
  //! @brief This particle's own randomly-assigned sway wave frequency.
  float m_frequency{ 0.5f };

private:
  //! @brief Rolls a new phase/frequency and launches the particle straight upward on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Particle sprite for a flame effect (candle/altar fire): particles rise, sway, and fade
//!        from white through Flame::m_final_flame_color before a sparse smoke tail finishes them off.
class FlameParticleSprite : public SpriteBase<detail::FlameParticle>
{
public:
  //! @brief Construct a new Flame object
  //! @param count Number of particles in this sprite
  FlameParticleSprite( size_t count );
  //! @brief Advances the flame simulation by one frame: sway, colour lerp, and smoke-phase fade.
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;

  //! @brief Draws each particle as a fan of triangles sized by its own `m_size` (see add_flame()'s `psize` param),
  //! rather than the base class's fixed dot size.
  //! @param target Render target to draw to.
  //! @param states Render states (transform/blend mode) to draw with.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;

  //! @brief Vertical pixel offset applied when positioning the flame sprite above its emitter.
  static const float kVerticalOffset;

  //! @brief Single wave time shared across all particles (currently unused by simulate(), which tracks per-particle wave time instead).
  float m_wave_time{ 0.f };
  //! @brief Colour the flame lerps to (from white) as each particle approaches the end of its flame phase.
  sf::Color m_final_flame_color{ 255, 0, 0 }; // red
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_FLAME_HPP__
