#ifndef SRC_SYSTEMS_THREATS_WISPSYSTEM_HPP__
#define SRC_SYSTEMS_THREATS_WISPSYSTEM_HPP__

#include <PathFinding/SmartPointers.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

#include <entt/entity/fwd.hpp>

namespace Game::Sys
{

//! @brief Spawns Wisp NPCs and drives their free-roaming pathfinding on the open navmesh,
//! retargeting them once they arrive at or get stuck heading towards their current target.
class WispSystem : public BaseSystem
{
public:
  //! @brief Construct a new Wisp System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  WispSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointer for the pathfinding navmesh
  //! @param open_navmesh Wisps roam freely, so pathfind on the open navmesh rather than the obstacle-aware one
  void init( const PathFinding::SpatialHashGridSharedPtr &open_navmesh ) { m_open_navmesh = open_navmesh; }

  //! @brief Pathfind existing wisps towards their target, retargeting any that arrive or get stuck
  //! @param dt Delta time since last update call
  void update( sf::Time dt );

  //! @brief Create a wisp NPC at a random location. Uses `m_open_navmesh` to pathfind freely around the scene.
  void spawn_wisp();

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Add a new pathfinding target for the wisp NPC.
  //! @param wisp_entt
  void reset_wisp_target( entt::entity wisp_entt );

  //! @brief track time since last pathfinding update
  sf::Time m_pathfinding_timer;

  //! @brief Delay the target reset once the pathfinding has comepleted.
  sf::Clock m_wisp_target_reset_clock;

  //! @brief Used for wisps to move freely within the level boundaries.
  PathFinding::SpatialHashGridWeakPtr m_open_navmesh;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_WISPSYSTEM_HPP__
