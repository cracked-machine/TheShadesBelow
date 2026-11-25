#ifndef SRC_SYSTEMS_RENDER_PAUSEOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_PAUSEOVERLAYSYSTEM_HPP__

#include <Systems/Render/BaseOverlaySystem.hpp>

#include <SFML/System/Time.hpp>

// clang-format off
namespace Game::Cmp::SceneSettings { struct Paused; }
// clang-format on

namespace Game::Sys
{

//! @brief Draws the transparent pause overlay: a "Paused" title and the music/effects volume sliders, drawn over the
//! frozen gameplay scene. The pause menu is open while the scene's Cmp::SceneSettings::Paused reason is PAUSEMENU; the
//! scene itself stops advancing the world while it is paused. OverlayArbitrationSystem opens and closes it on Events::OverlayEvent PAUSE_TOGGLE.
class PauseOverlaySystem : public BaseOverlaySystem
{
public:
  //! @brief Construct a new Pause Overlay System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  PauseOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief True while the pause menu is open (Cmp::SceneSettings::Paused::Reason::PAUSEMENU). False for scenes without a
  //! Cmp::SceneSettings::Paused component, or while paused by a tooltip.
  [[nodiscard]] bool is_open();

  //! @brief Open the pause menu: cancels any bow draw and pauses the sound effects. No-op if already paused for any
  //! reason, so it never opens during a pausing tooltip.
  void open();

  //! @brief Close the pause menu, saving the volume settings and resuming the sound effects. No-op if it is not open.
  void close();

  //! @brief Close the pause menu without saving, e.g. when the scene is left while paused. Resumes the sound effects.
  //! No-op if it is not open.
  void force_resume();

  //! @brief Draw the pause overlay and its volume sliders, if paused, and apply the slider values.
  //! @param dt
  void update( sf::Time dt ) override;

private:
  //! @brief The scene's pause setting.
  //! @return Pointer to the component, or nullptr if the scene has none.
  Cmp::SceneSettings::Paused *find_paused();
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_PAUSEOVERLAYSYSTEM_HPP__
