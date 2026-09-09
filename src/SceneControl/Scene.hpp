#ifndef SRC_SCENECONTROL_SCENE_HPP__
#define SRC_SCENECONTROL_SCENE_HPP__

#include <PathFinding/SmartPointers.hpp>
#include <SceneControl/IScene.hpp>
#include <SceneControl/SmartPointers.hpp>

// clang-format off
namespace PathFinding { class SpatialHashGrid; }
// clang-format on

namespace Game::Scene
{

//! @brief Base class for all scenes that process input events of a specific type
//! @tparam InputEventType The type of input event to process. This essentially means we can "bake in"
//!         the call to `m_nav_event_dispatcher.enqueue<InputEventType>()` after each update, so that the
//!         specialization of this class template doesn't have to remember to do that.
template <typename InputEventType>
class Scene : public IScene
{
public:
  //! @brief Construct a new Scene object
  //! @param nav_event_dispatcher Used to notify the SceneInputRouter when new input events are available
  explicit Scene( entt::dispatcher &nav_event_dispatcher )
      : m_nav_event_dispatcher( nav_event_dispatcher )
  {
  }

  //! @brief Destroy the Scene object
  virtual ~Scene() = default;

  // Prevent move/copy of a scene object; Use a unique ptr.
  Scene( const Scene &other ) = delete;
  Scene &operator=( const Scene & ) = delete;
  Scene( Scene &&other ) = delete;
  Scene &operator=( Scene && ) = delete;

  //! @brief Public API for updating the scene
  //! @param dt The time elapsed since the last update
  void update( sf::Time dt ) override
  {
    // call the user-overridden update function
    do_update( dt );

    // Notify SceneInputRouter that there may be new input events to process
    m_nav_event_dispatcher.enqueue( InputEventType() );
  }

  //! @brief Get the scene's grid of positions reserved from procgen/algorithmic changes.
  //! @return PathFinding::SpatialHashGridSharedPtr
  PathFinding::SpatialHashGridSharedPtr get_reserved_sm() override { return m_reserved_sm; }

protected:
  //! @brief Blocks NPC pathfinding on obstacles, plants, multiblocks, etc...
  PathFinding::SpatialHashGridSharedPtr m_generic_npc_navmesh;
  //! @brief Same as generic but excludes plants
  PathFinding::SpatialHashGridSharedPtr m_ghost_navmesh;
  //! @brief The entire game area for uninhibited NPC pathfinding e.g. wisps
  PathFinding::SpatialHashGridSharedPtr m_open_navmesh;
  //! @brief Player navmesh for blocking movement
  PathFinding::SpatialHashGridSharedPtr m_player_navmesh;
  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridSharedPtr m_reserved_sm;

  //! @brief Deserialised JSON scene data. See res/scenes
  SceneMapSharedPtr m_scene_data;

  //! @brief Flag when player has just spawned in the scene.
  bool m_just_spawned{ true };

private:
  //! @brief Non-owning ref to the dispatcher for routing scene-specific input events (keyboard/mouse) to SceneInputRouter.
  entt::dispatcher &m_nav_event_dispatcher;
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_SCENE_HPP__
