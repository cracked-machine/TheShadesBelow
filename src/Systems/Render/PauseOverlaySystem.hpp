#ifndef SRC_SYSTEMS_RENDER_PAUSEOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_PAUSEOVERLAYSYSTEM_HPP__

#include <Components/SceneSettings/Paused.hpp>
#include <Systems/Render/BaseOverlaySystem.hpp>

#include <SFML/System/Time.hpp>

namespace Game::Sys
{

//! @brief Draws the transparent pause overlay: a "Paused" title and the music/effects volume sliders, drawn over the
//! frozen gameplay scene. The pause state is the scene's Cmp::SceneSettings::Paused component; the scene itself stops
//! advancing the world while it is set. OverlayArbitrationSystem opens and closes it on Events::OverlayEvent PAUSE_TOGGLE.
class PauseOverlaySystem : public BaseOverlaySystem
{
public:
  //! @brief Construct a new Pause Overlay System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  PauseOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief True while the current scene is paused. False for scenes without a Cmp::SceneSettings::Paused component.
  [[nodiscard]] bool is_open();

  //! @brief Pause the scene: cancels any bow draw and pauses the sound effects. No-op if already paused.
  void open();

  //! @brief Resume the scene, saving the volume settings and resuming the sound effects. No-op if not paused.
  void close();

  //! @brief Unpause without saving, e.g. when the scene is left while paused. Resumes the sound effects. No-op if not paused.
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
