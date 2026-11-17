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

  //! @brief Break the targetted Cmp::LootContainer with a tool via Events::UpdateDmgEvent and DestroyLootContainerEvent
  void check_player_smash_pot();

  //! @brief Break the targetted Curse Tablet (Cmp::WorldItem) with a tool via Events::UpdateDmgEvent
  void check_player_smash_tablet();

  //! @brief Apply wear to the equipped tool via Events::UpdateDmgEvent. No-op if the tool is already broken.
  //! @tparam ShovelWear Persistent wear component used when the shovel is equipped
  template <typename ShovelWear>
  void apply_tool_wear();

  //! @brief Banish up to `max_count` NPCs of type NpcType that are visible in the world view, nearest to `origin` first.
  //! @tparam NpcType NPC tag component, e.g. Cmp::Npc::Ghost
  //! @param origin Distances are measured from here, e.g. the player position.
  //! @param max_count Maximum number of NPCs to banish.
  template <typename NpcType>
  void banish_nearest_npcs( sf::Vector2f origin, int max_count );

  //! @brief With the shovel equipped, kill the mouse-selected skeleton or Burryman next to the player.
  void check_player_melee_npc_kill();

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
