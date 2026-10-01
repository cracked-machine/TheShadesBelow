#ifndef SRC_SCENECONTROL_SCENES_RUINSCENEUPPERFLOOR_HPP__
#define SRC_SCENECONTROL_SCENES_RUINSCENEUPPERFLOOR_HPP__

#include <Audio/SoundBank.hpp>
#include <SFML/System/Vector2.hpp>
#include <SceneControl/Events/ProcessRuinSceneUpperInputEvent.hpp>
#include <SceneControl/Scene.hpp>
#include <Sprites/VertexFloor.hpp>
#include <Systems/Stores/SystemStore.hpp>

namespace Game::Scene
{

//! @brief The upper floor of the ruin dungeon: a procedurally generated interior level with a hexagram
//! puzzle and a shadow-hand hazard, that descends into RuinSceneLowerFloor via the floor access hitbox.
class RuinSceneUpperFloor : public Scene<Events::ProcessRuinSceneUpperInputEvent>
{
public:
  //! @brief Construct a new RuinSceneUpperFloor object
  //! @param sound_bank Shared sound bank used to play/stop ruin-related audio
  //! @param system_store Store of game systems the scene drives each update
  //! @param nav_event_dispatcher Used to notify the SceneInputRouter when new input events are available
  RuinSceneUpperFloor( Audio::SoundBank &sound_bank, Sys::Store &system_store, entt::dispatcher &nav_event_dispatcher )
      : Scene( nav_event_dispatcher ),
        m_sound_bank( sound_bank ),
        m_sys( system_store )
  {
  }

  //! @brief Procedurally generates the ruin upper floor (hexagram puzzle, floor access hitbox, navmeshes,
  //! floor tiles) and initializes persistent state
  void on_init() override;

  //! @brief Starts ruin ambience/music, snaps the player to grid on first spawn, and resets the floor-access cooldown
  void on_enter() override;

  //! @brief Stops ruin ambience/music and clears the registry
  void on_exit() override;

  //! @brief Get the name of the scene
  //! @return std::string "RuinSceneUpperFloor"
  [[nodiscard]] std::string get_name() const override { return "RuinSceneUpperFloor"; }

  //! @brief Get the registry object owned by the scene
  //! @return entt::registry&
  entt::registry &registry() override;

protected:
  //! @brief Advances animation, NPC, footstep, loot, floor-access, curse, shadow-hand, and player systems
  //! for one frame and renders the game
  //! @param dt The time elapsed since the last update
  void do_update( sf::Time dt ) override;

private:
  //! @brief Re-wires the freshly (re)created navmeshes into the systems that depend on them
  void reinit_navmesh();

  //! @brief Shared sound bank used to play/stop ruin-related audio
  Audio::SoundBank &m_sound_bank;

  //! @brief Store of game systems the scene drives each update
  Sys::Store &m_sys;
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_SCENES_RUINSCENEUPPERFLOOR_HPP__
