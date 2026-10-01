#ifndef SRC_SCENECONTROL_SCENES_SHOPSCENE_HPP__
#define SRC_SCENECONTROL_SCENES_SHOPSCENE_HPP__

#include <Components/Shop/Inventory.hpp>
#include <SceneControl/Events/ProcessShopSceneInputEvent.hpp>
#include <SceneControl/Scene.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Sprites/VertexFloor.hpp>

#include <SFML/System/Vector2.hpp>

// clang-format off
namespace Game::Sys { class Store; }
namespace Game::Audio { class SoundBank; }
// clang-format on

namespace Game::Scene
{

//! @brief The shop scene: a small level where the player can buy items from a shopkeeper NPC, whose
//! proximity opens/closes the shop inventory overlay.
class ShopScene : public Scene<Events::ProcessShopSceneInputEvent>
{
public:
  //! @brief Construct a new ShopScene object
  //! @param sound_bank Shared sound bank used to play/stop shop-related audio
  //! @param system_store Store of game systems the scene drives each update
  //! @param nav_event_dispatcher Used to notify the SceneInputRouter when new input events are available
  ShopScene( Audio::SoundBank &sound_bank, Sys::Store &system_store, entt::dispatcher &nav_event_dispatcher )
      : Scene( nav_event_dispatcher ),
        m_sound_bank( sound_bank ),
        m_sys( system_store )
  {
  }

  //! @brief Builds the shop level (level geometry, shop inventory, floor tiles, navmeshes) and initializes persistent state
  void on_init() override;
  //! @brief Restores player position on first entry and lights the player's candle if carried
  void on_enter() override;
  //! @brief Clears the registry and stops footstep audio
  void on_exit() override;
  //! @brief Get the name of the scene
  //! @return std::string "ShopScene"
  std::string get_name() const override { return "ShopScene"; }

  //! @brief Get the registry object owned by the scene
  //! @return entt::registry&
  entt::registry &registry() override;

  //! @brief Enables the shop inventory UI and marks the overlay as open
  void open_overlay();
  //! @brief Disables the shop inventory UI and marks the overlay as closed
  void close_overlay();
  //! @brief Whether the shop inventory overlay is currently open
  //! @return bool true if the overlay is open
  bool is_overlay_open() const { return m_overlay_open; }

protected:
  //! @brief Advances animation, footstep, and particle systems; opens/closes the shop overlay based on
  //! shopkeeper proximity; and updates the player and renders the game
  //! @param dt The time elapsed since the last update
  void do_update( [[maybe_unused]] sf::Time dt ) override;

private:
  //! @brief Re-wires the freshly (re)created navmeshes into the systems that depend on them
  void reinit_navmesh();

  //! @brief Shared sound bank used to play/stop shop-related audio
  Audio::SoundBank &m_sound_bank;

  //! @brief Store of game systems the scene drives each update
  Sys::Store &m_sys;

  //! @brief Vertex-based floor tile mesh for the shop level
  Sprites::Containers::VertexFloor m_floormap;

  //! @brief Whether the shop inventory overlay is currently open
  bool m_overlay_open{ false };
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_SCENES_SHOPSCENE_HPP__
