#ifndef SRC_SCENECONTROL_SCENES_PAUSEDMENUSCENE_HPP__
#define SRC_SCENECONTROL_SCENES_PAUSEDMENUSCENE_HPP__

#include <SceneControl/Events/ProcessPausedMenuSceneInputEvent.hpp>
#include <SceneControl/Scene.hpp>

// clang-format off
namespace Game::Sys { class Store; }
namespace Game::Audio { class SoundBank; }
// clang-format on

namespace Game::Scene
{

//! @brief The pause menu overlay: renders the pause menu and persists volume settings while paused.
//! Gameplay is frozen because SceneManager only updates the top scene, so the gameplay scene beneath
//! receives no dt while this scene is active.
class PausedMenuScene : public Scene<Events::ProcessPausedMenuSceneInputEvent>
{
public:
  //! @brief Construct a new PausedMenuScene object
  //! @param sound_bank Shared sound bank used to update music/effects volume while paused
  //! @param system_store Store of game systems the scene drives each update
  //! @param nav_event_dispatcher Used to notify the SceneInputRouter when new input events are available
  PausedMenuScene( Audio::SoundBank &sound_bank, Sys::Store &system_store, entt::dispatcher &nav_event_dispatcher )
      : Scene( nav_event_dispatcher ),
        m_sound_bank( sound_bank ),
        m_sys( system_store )
  {
  }

  //! @brief Logs scene initialization; no world entities are created for this scene
  void on_init() override;

  //! @brief Reloads persistent state and pauses any playing sound effects
  void on_enter() override;

  //! @brief Saves persistent state (volume settings changed via UI) and resumes paused sound effects
  void on_exit() override;

  //! @brief Get the name of the scene
  //! @return std::string "PausedMenuScene"
  [[nodiscard]] std::string get_name() const override { return "PausedMenuScene"; }

  //! @brief Get the registry object owned by the scene
  //! @return entt::registry&
  entt::registry &registry() override;

protected:
  //! @brief Renders the pause menu, saves persistent settings, and applies the current music/effects volume
  //! @param dt The time elapsed since the last update
  void do_update( [[maybe_unused]] sf::Time dt ) override;

private:
  //! @brief Shared sound bank used to update music/effects volume while paused
  Audio::SoundBank &m_sound_bank;

  //! @brief Store of game systems the scene drives each update
  Sys::Store &m_sys;
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_SCENES_PAUSEDMENUSCENE_HPP__
