#ifndef SRC_SCENECONTROL_SCENES_LEVELCOMPLETESCENE_HPP__
#define SRC_SCENECONTROL_SCENES_LEVELCOMPLETESCENE_HPP__

#include <SceneControl/Events/ProcessLevelCompleteSceneInputEvent.hpp>
#include <SceneControl/Scene.hpp>

// clang-format off
namespace Game::Sys { class Store; }
namespace Game::Audio { class SoundBank; }
// clang-format on

namespace Game::Scene
{

//! @brief The victory screen shown when the player wins: periodically converts any remaining collected
//! cadavers and unspent anima into wealth while the victory screen is displayed. The player can only
//! continue once both have run out, so every level starts with zero anima.
class LevelCompleteScene : public Scene<Events::ProcessLevelCompleteSceneInputEvent>
{
public:
  //! @brief Construct a new LevelCompleteScene object
  //! @param sound_bank Shared sound bank used to play/stop audio for this scene
  //! @param system_store Store of game systems the scene drives each update
  //! @param nav_event_dispatcher Used to notify the SceneInputRouter when new input events are available
  LevelCompleteScene( Audio::SoundBank &sound_bank, Sys::Store &system_store, entt::dispatcher &nav_event_dispatcher )
      : Scene( nav_event_dispatcher ),
        m_sound_bank( sound_bank ),
        m_sys( system_store )
  {
  }

  //! @brief No-op: no world entities need to be created for this scene
  void on_init() override;

  //! @brief Reloads persistent state and switches from graveyard music to title music
  void on_enter() override;

  //! @brief Clears the registry and stops footstep audio
  void on_exit() override;

  //! @brief Get the name of the scene
  //! @return std::string "LevelCompleteScene"
  [[nodiscard]] std::string get_name() const override { return "LevelCompleteScene"; }

  //! @brief Get the registry object owned by the scene
  //! @return entt::registry&
  entt::registry &registry() override;

protected:
  //! @brief Every second, converts one collected cadaver into 10 wealth, and every kAnimaTickSeconds converts one
  //! anima into 1 wealth (playing a loot sound), until none remain, then renders the victory screen
  //! @param dt The time elapsed since the last update
  void do_update( [[maybe_unused]] sf::Time dt ) override;

private:
  //! @brief Shared sound bank used to play/stop audio for this scene
  Audio::SoundBank &m_sound_bank;

  //! @brief Store of game systems the scene drives each update
  Sys::Store &m_sys;

  //! @brief Accumulates elapsed time between cadaver-to-wealth score conversion ticks
  sf::Time m_scorecheck_accumulator;

  //! @brief Accumulates elapsed time between anima-to-wealth conversion ticks
  sf::Time m_anima_accumulator;

  //! @brief Seconds between converting each anima point into wealth
  static constexpr float kAnimaTickSeconds = 0.1f;

  //! @brief Wealth added so far by converting cadavers, shown on the victory screen
  int m_cadaver_wealth{ 0 };

  //! @brief Wealth added so far by converting anima, shown on the victory screen
  int m_anima_wealth{ 0 };
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_SCENES_LEVELCOMPLETESCENE_HPP__
