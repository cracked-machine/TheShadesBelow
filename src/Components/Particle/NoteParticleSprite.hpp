#ifndef SRC_COMPONENTS_PARTICLE_NOTEPARTICLESPRITE_HPP__
#define SRC_COMPONENTS_PARTICLE_NOTEPARTICLESPRITE_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/Color.hpp>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual particle for the NoteParticleSprite effect: rises from the emitter, swaying side to
//! side for its whole life, fading out as it rises, in a randomly rolled color re-chosen each respawn.
struct NoteParticle : public Cmp::Particle::ParticleBase
{
  //! @brief Elapsed time (seconds) since this particle's own wave motion last reset, independent of the other particles.
  float m_wave_time{ 0.f };
  //! @brief This particle's own randomly-assigned sway wave phase.
  float m_phase{ 0.f };
  //! @brief This particle's own randomly-assigned sway wave frequency.
  float m_frequency{ 0.5f };
  //! @brief This particle's randomly-rolled base color, re-rolled every time it respawns (see emit()).
  sf::Color m_base_color{ 255, 255, 255 };

private:
  //! @brief Rolls a new phase/frequency/color and launches the particle straight upward on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Particle sprite for floating music notes: each note rises continuously from the emitter (the
//! player's feet), swaying gently side to side for its whole life, in a random color that fades out as it
//! rises, respawning forever.
class NoteParticleSprite : public SpriteBase<detail::NoteParticle>
{
public:
  //! @brief Tag of the player's floating music-note particles, followed every frame to the player's feet.
  static constexpr std::string_view kPlayerNotesTag = "player.flute.particle.notes";

  //! @brief Construct a new NoteParticleSprite object
  //! @param count Number of particles in this sprite
  NoteParticleSprite( size_t count );
  //! @brief Advances the NoteParticleSprite simulation by one frame (rise, sway, and lifetime-based fade).
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;

  //! @brief Draw each particle as a music-note glyph (notehead, stem and flag) in screen view.
  //! @param target
  //! @param states
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_NOTEPARTICLESPRITE_HPP__
