#ifndef SRC_SCENECONTROL_SCENES_BARBERSURGEONSCENE_HPP__
#define SRC_SCENECONTROL_SCENES_BARBERSURGEONSCENE_HPP__

#include <SFML/System/Vector2.hpp>
#include <SceneControl/Events/ProcessBarberSurgeonSceneInputEvent.hpp>
#include <SceneControl/Scene.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Sprites/VertexFloor.hpp>

// clang-format off
namespace Game::Sys { class Store; }
namespace Game::Audio { class SoundBank; }
// clang-format on

namespace Game::Scene
{

//! @brief The barber surgeon scene: a small level where the player's damage cooldown is force-expired,
//! reachable from the graveyard.
class BarberSurgeonScene : public Scene<Events::ProcessBarberSurgeonSceneInputEvent>
{
public:
  //! @brief Construct a new BarberSurgeonScene object
  //! @param sound_bank Shared sound bank used to play/stop barber-surgeon-related audio
  //! @param system_store Store of game systems the scene drives each update
  //! @param nav_event_dispatcher Used to notify the SceneInputRouter when new input events are available
  BarberSurgeonScene( Audio::SoundBank &sound_bank, Sys::Store &system_store, entt::dispatcher &nav_event_dispatcher )
      : Scene( nav_event_dispatcher ),
        m_sound_bank( sound_bank ),
        m_sys( system_store )
  {
  }

  //! @brief Builds the barber surgeon level (level geometry, floor tiles, navmeshes) and initializes persistent state
  void on_init() override;

  //! @brief Restores player position on first entry, starts the (shared) healing spring music, locks player movement
  //! until loaded, and force-expires the player's damage cooldown
  void on_enter() override;

  //! @brief Stops the healing spring music and footstep audio, clears the registry, and delays for a forced loading screen
  void on_exit() override;

  //! @brief Get the name of the scene
  //! @return std::string "BarberSurgeonScene"
  [[nodiscard]] std::string get_name() const override { return "BarberSurgeonScene"; }

  //! @brief Get the registry object owned by the scene
  //! @return entt::registry&
  entt::registry &registry() override;

protected:
  //! @brief Advances animation, NPC, footstep, loot, barber surgeon exit-collision, player, and particle
  //! systems for one frame and renders the game
  //! @param dt The time elapsed since the last update
  void do_update( sf::Time dt ) override;

private:
  //! @brief Re-wires the freshly (re)created navmeshes into the systems that depend on them
  void reinit_navmesh();

  //! @brief Shared sound bank used to play/stop barber-surgeon-related audio
  Audio::SoundBank &m_sound_bank;

  //! @brief Store of game systems the scene drives each update
  Sys::Store &m_sys;
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_SCENES_BARBERSURGEONSCENE_HPP__
