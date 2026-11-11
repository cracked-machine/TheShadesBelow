#ifndef SRC_SYSTEMS_RENDER_PAUSEOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_PAUSEOVERLAYSYSTEM_HPP__

#include <Events/PlayerActionEvent.hpp>
#include <Systems/Render/RenderSystem.hpp>

#include <SFML/System/Time.hpp>

namespace Game::Sys
{

//! @brief Toggles and draws the transparent pause overlay: a "Paused" title and the music/effects volume sliders, drawn
//! over the frozen gameplay scene. The pause state is the scene's Cmp::SceneSettings::Paused component; the scene itself
//! stops advancing the world while it is set.
class PauseOverlaySystem : public RenderSystem
{
public:
  //! @brief Construct a new Pause Overlay System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  PauseOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief True while the current scene is paused. False for scenes without a Cmp::SceneSettings::Paused component.
  [[nodiscard]] bool is_paused() const;

  //! @brief Draw the pause overlay and apply the volume sliders, if paused. Pumps the frame's ImGui frame while paused.
  //! @param dt Real frame time, for ImGui.
  void render( sf::Time dt );

  //! @brief Unpause without saving, e.g. when the scene is left while paused. Resumes the sound effects. No-op if not paused.
  void force_resume();

private:
  //! @brief Toggle the pause on PAUSE_TOGGLE. Pausing cancels any bow draw and pauses sound effects; resuming saves the
  //! volume settings and resumes them. Ignored while the grimoire is open: both overlays pump the one ImGui frame allowed per frame.
  //! @param ev
  void on_player_action( Events::PlayerActionEvent ev );

  //! @brief Whether the overlay pumped an ImGui frame last frame, so the frame it closes on can pump one more to
  //! consume the events forwarded to ImGui while it was open.
  bool m_imgui_active{ false };
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_PAUSEOVERLAYSYSTEM_HPP__
