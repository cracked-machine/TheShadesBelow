#ifndef SRC_SCENECONTROL_SCENES_GRAVEYARDSCENE_HPP__
#define SRC_SCENECONTROL_SCENES_GRAVEYARDSCENE_HPP__

#include <SceneControl/Events/ProcessGraveyardSceneInputEvent.hpp>
#include <SceneControl/Scene.hpp>
#include <Sprites/VertexFloor.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/System/Time.hpp>

// clang-format off
namespace Game::Sys { class Store; }
namespace Game::Audio { class SoundBank; }
// clang-format on

namespace Game::Scene
{

//! @brief The graveyard hub scene: the main procedurally generated exterior level that connects to the
//! crypt, ruins, healing spring, and shop, and hosts NPCs, hazards, plants, and the grimoire overlay.
class GraveyardScene : public Scene<Events::ProcessGraveyardSceneInputEvent>
{
public:
  //! @brief Construct a new GraveyardScene object
  //! @param sound_bank Shared sound bank used to play/stop graveyard-related audio
  //! @param system_store Store of game systems the scene drives each update
  //! @param nav_event_dispatcher Used to notify the SceneInputRouter when new input events are available
  //! @param sprite_Factory Factory used to create sprites for entities spawned in this scene
  GraveyardScene( Audio::SoundBank &sound_bank, Sys::Store &system_store, entt::dispatcher &nav_event_dispatcher,
                  Sprites::SpriteFactory &sprite_Factory )
      : Scene( nav_event_dispatcher ),
        m_sound_bank( sound_bank ),
        m_sys( system_store ),
        m_sprite_factory( sprite_Factory )
  {
  }

  //! @brief Procedurally generates the graveyard level (obstacles, plants, hazards, NPCs, loot, navmeshes,
  //! floor tiles) and initializes persistent state
  void on_init() override;

  //! @brief Restores the player to their last graveyard position (or start position on first spawn),
  //! starts graveyard music, and resets the scene-exit cooldown
  void on_enter() override;

  //! @brief Clears the registry, stops graveyard music/footstep audio, and clears floor tiles
  void on_exit() override;

  //! @brief Get the name of the scene
  //! @return std::string "GraveyardScene"
  [[nodiscard]] std::string get_name() const override { return "GraveyardScene"; }

  //! @brief Get the registry object owned by the scene
  //! @return entt::registry&
  entt::registry &registry() override;

protected:
  //! @brief Advances all graveyard-related systems (hazards, NPCs, wisps, watchman, wormholes, altar,
  //! healing spring, ruin/crypt entrances, lightning, grimoire, arrows, particles) for one frame and renders the game
  //! @param dt The time elapsed since the last update
  void do_update( sf::Time dt ) override;

private:
  //! @brief Shared sound bank used to play/stop graveyard-related audio
  Audio::SoundBank &m_sound_bank;

  //! @brief Store of game systems the scene drives each update
  Sys::Store &m_sys;

  //! @brief Factory used to create sprites for entities spawned in this scene
  Sprites::SpriteFactory &m_sprite_factory;

  //! @brief Clock measuring elapsed time since the scene was last entered, used to gate exit collision checks
  sf::Clock m_scene_exit_cooldown{};

  //! @brief Minimum time after entering the scene before exit collisions (e.g. crypt entrance) are checked
  sf::Time m_scene_exit_cooldown_time{ sf::seconds( 2 ) };

  //! @brief Whether the grimoire overlay UI is currently open
  bool m_grimoire_overlay_open{ false };

  //! @brief Re-wires the freshly (re)created navmeshes into the systems that depend on them
  void reinit_system_spatial_maps();

  //! @brief Spatial index of static (never moved after creation) renderable entities, queried by
  //! ZOrderQueue::refresh() instead of a full-registry scan. First built after level
  //! generation finishes in on_init(), then periodically rebuilt from scratch in do_update() (see
  //! m_render_position_grid_rebuild_clock) so runtime-created static entities - dropped items,
  //! replanted trees, placed bombs, footstep decals - eventually get indexed too, without needing
  //! precise insert()/remove() calls threaded through every system that can create/destroy one.
  PathFinding::SpatialHashGridSharedPtr m_render_position_sm;

  //! @brief Rate-limits m_render_position_sm rebuilds; see its doc comment.
  sf::Clock m_render_position_grid_rebuild_clock;

  //! @brief How often m_render_position_sm is rebuilt from scratch. Trades off staleness (a newly
  //! created static entity won't render until the next rebuild) against cost (a full registry scan) -
  //! short enough that the delay isn't noticeable, long enough to stay cheap amortized per frame.
  static constexpr sf::Time kRenderPositionGridRebuildInterval{ sf::seconds( 0.25f ) };
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_SCENES_GRAVEYARDSCENE_HPP__
