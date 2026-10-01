#ifndef SRC_SCENECONTROL_SCENES_RUINSCENELOWERFLOOR_HPP__
#define SRC_SCENECONTROL_SCENES_RUINSCENELOWERFLOOR_HPP__

#include <Audio/SoundBank.hpp>
#include <SceneControl/Events/ProcessRuinSceneLowerInputEvent.hpp>
#include <SceneControl/Scene.hpp>
#include <Sprites/SpriteMetaType.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Sprites/VertexFloor.hpp>
#include <Systems/Stores/SystemStore.hpp>

namespace Game::Sprites
{
class SpriteSheet;
} // namespace Game::Sprites

namespace Game::Scene
{

//! @brief The lower floor of the ruin dungeon: a procedurally generated interior level reachable either
//! from the graveyard door or by descending from RuinSceneUpperFloor, with cobwebs, curse hazards, and spiders.
class RuinSceneLowerFloor : public Scene<Events::ProcessRuinSceneLowerInputEvent>
{
public:
  //! @brief How the player is entering the lower floor, used by on_enter() to decide the spawn position/state
  enum class EntryMode {
    //! @brief Entering via the graveyard door; spawn at the scene's persisted start position
    FROM_DOOR,
    //! @brief Descending from RuinSceneUpperFloor; snap to grid and clear the damage cooldown
    FROM_UPPER_FLOOR
  };

  //! @brief Construct a new RuinSceneLowerFloor object
  //! @param sound_bank Shared sound bank used to play/stop ruin-related audio
  //! @param system_store Store of game systems the scene drives each update
  //! @param nav_event_dispatcher Used to notify the SceneInputRouter when new input events are available
  RuinSceneLowerFloor( Audio::SoundBank &sound_bank, Sys::Store &system_store, entt::dispatcher &nav_event_dispatcher )
      : Scene( nav_event_dispatcher ),
        m_sound_bank( sound_bank ),
        m_sys( system_store )
  {
  }

  //! @brief Procedurally generates the ruin lower floor (obstacles, cobwebs, rune markers, navmeshes, floor tiles)
  //! and initializes persistent state
  void on_init() override;

  //! @brief Positions the player according to m_entry_mode, starts ruin ambience/music, locks player
  //! movement until loaded, and re-wires navmeshes
  void on_enter() override;

  //! @brief Stops ruin ambience/music, clears the registry, and delays for a forced loading screen
  void on_exit() override;

  //! @brief Get the name of the scene
  //! @return std::string "RuinSceneLowerFloor"
  [[nodiscard]] std::string get_name() const override { return "RuinSceneLowerFloor"; }

  //! @brief Get the registry object owned by the scene
  //! @return entt::registry&
  entt::registry &registry() override;

  //! @brief Records how the player is entering the scene, consulted by on_enter() on the next entry
  //! @param entry_mode Whether the player is entering from the door or descending from the upper floor
  void set_entry_mode( EntryMode entry_mode ) { m_entry_mode = entry_mode; }

protected:
  //! @brief Advances animation, NPC, footstep, loot, floor-access, curse, and player systems for one frame
  //! and renders the game
  //! @param dt The time elapsed since the last update
  void do_update( [[maybe_unused]] sf::Time dt ) override;

private:
  //! @brief Re-wires the freshly (re)created navmeshes into the systems that depend on them
  void reinit_navmesh();

  //! @brief Shared sound bank used to play/stop ruin-related audio
  Audio::SoundBank &m_sound_bank;

  //! @brief Store of game systems the scene drives each update
  Sys::Store &m_sys;

  //! @brief How the player is currently entering the scene; set via set_entry_mode() before the scene is switched to
  EntryMode m_entry_mode;
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_SCENES_RUINSCENELOWERFLOOR_HPP__
