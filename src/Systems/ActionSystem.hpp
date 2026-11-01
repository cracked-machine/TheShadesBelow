#ifndef SRC_SYSTEMS_ACTIONSYSTEM_HPP__
#define SRC_SYSTEMS_ACTIONSYSTEM_HPP__

#include <Events/PlayerActionEvent.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::PathFinding
{
class SpatialHashGrid;
}

namespace Game::Sys
{

//! @brief Handles player actions within the game. This system is mainly event-driven, responding to
//! Events::PlayerActionEvent.
class ActionSystem : public BaseSystem
{
public:
  //! @brief Construct a new Action System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  ActionSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointer for the pathfinding navmesh
  //! @param npc_navmesh
  //! @param player_navmesh
  //! @param ghost_navmesh
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh, const PathFinding::SpatialHashGridSharedPtr &player_navmesh,
             const PathFinding::SpatialHashGridSharedPtr &ghost_navmesh = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr )
  {
    m_npc_navmesh = npc_navmesh;
    m_player_navmesh = player_navmesh;
    m_ghost_navmesh = ghost_navmesh;
    m_reserved_sm = reserved_sm;
  }

  //! @brief frame updates
  //! @param dt
  void update( sf::Time dt );

private:
  //! @brief Event handler for player actions
  //! @param event
  void on_player_action( const Events::PlayerActionEvent &event );

  //! @brief Uses Cmp::SelectedPosition to mark the targetted Cmp::Obstacle then applies digging damage until the obstacle is destroyed.
  void check_player_dig_obstacle_collision();

  //! @brief Break the targetted Cmp::LootContainer with a tool via Events::DestroyLootContainerEvent (see
  //! LootSystem::on_destroy_loot_container_event)
  void check_player_smash_pot();

  //! @brief If player is carry suitable weapon did the action event occur in NPC vicinity?
  void check_player_axe_npc_kill();

  //! @brief Uses Cmp::SelectedPosition to mark the targetted Cmp::Obstacle/Cmp::Moveable entity.
  void select_moveable_obstacle();

  //! @brief all grid positions that block NPC pathfinding
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief All grid positions that block ghost NPC pathfinding
  PathFinding::SpatialHashGridWeakPtr m_ghost_navmesh;

  //! @brief All grid positions that block player movement
  PathFinding::SpatialHashGridWeakPtr m_player_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;
};
} // namespace Game::Sys

#endif // SRC_SYSTEMS_ACTIONSYSTEM_HPP__
