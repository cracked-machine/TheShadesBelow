#ifndef SRC_SCENECONTROL_SCENES_CRYPTSCENE_HPP__
#define SRC_SCENECONTROL_SCENES_CRYPTSCENE_HPP__

#include <SceneControl/Events/ProcessCryptSceneInputEvent.hpp>
#include <SceneControl/Scene.hpp>

#include <Sprites/VertexFloor.hpp>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Vector2.hpp>

// clang-format off
namespace Game::Sys { class Store; }
namespace Game::Audio { class SoundBank; }
// clang-format on

namespace Game::Scene
{

//! @brief The crypt dungeon scene: a procedurally generated set of rooms the player explores,
//! fighting NPCs and triggering shockwave/priest weapon effects, until they reach the end room.
class CryptScene : public Scene<Events::ProcessCryptSceneInputEvent>
{
public:
  //! @brief Construct a new CryptScene object
  //! @param sound_bank Shared sound bank used to play/stop crypt-related audio
  //! @param system_store Store of game systems the scene drives each update
  //! @param nav_event_dispatcher Used to notify the SceneInputRouter when new input events are available
  CryptScene( Audio::SoundBank &sound_bank, Sys::Store &system_store, entt::dispatcher &nav_event_dispatcher )
      : Scene( nav_event_dispatcher ),
        m_sound_bank( sound_bank ),
        m_sys( system_store )
  {
  }

  //! @brief Builds the crypt level (rooms, navmeshes, floor tiles, candle flame particles) and initializes persistent state
  void on_init() override;

  //! @brief Restores player position and starts the crypt shuffle timer on first entry, and locks player movement until the scene has loaded
  void on_enter() override;

  //! @brief Destroys the crypt shuffle timer, stops footstep audio, removes the player's extra life, and delays for a forced loading screen
  void on_exit() override;

  //! @brief Get the name of the scene
  //! @return std::string "CryptScene"
  std::string get_name() const override { return "CryptScene"; }

  //! @brief Get the registry object owned by the scene
  //! @return entt::registry&
  entt::registry &registry() override;

protected:
  //! @brief Advances all crypt-related systems (animation, NPCs, footsteps, loot, shockwaves, player, passages,
  //! particles) for one frame and renders the game
  //! @param dt The time elapsed since the last update
  void do_update( [[maybe_unused]] sf::Time dt ) override;

private:
  //! @brief Re-wires the freshly (re)created navmeshes into the systems that depend on them
  void reinit_navmesh();

  //! @brief Shared sound bank used to play/stop crypt-related audio
  Audio::SoundBank &m_sound_bank;

  //! @brief Store of game systems the scene drives each update
  Sys::Store &m_sys;
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_SCENES_CRYPTSCENE_HPP__
