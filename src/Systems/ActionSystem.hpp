#ifndef SRC_SYSTEMS_ACTIONSYSTEM_HPP__
#define SRC_SYSTEMS_ACTIONSYSTEM_HPP__

#include <Events/PlayerActionEvent.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/StoreKey.hpp>

namespace Game::PathFinding
{
class SpatialHashGrid;
}

namespace Game::Cmp
{
class PlantMultiBlock;
struct UUID;
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
  //! @param loot_sm loot drops, so foraged items don't drop on them
  void init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh, const PathFinding::SpatialHashGridSharedPtr &player_navmesh,
             const PathFinding::SpatialHashGridSharedPtr &ghost_navmesh = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr, const PathFinding::SpatialHashGridSharedPtr &loot_sm = nullptr )
  {
    m_npc_navmesh = npc_navmesh;
    m_player_navmesh = player_navmesh;
    m_ghost_navmesh = ghost_navmesh;
    m_reserved_sm = reserved_sm;
    m_loot_sm = loot_sm;
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

  //! @brief Uses Cmp::SelectedPosition to mark the targetted Cmp::PlantMultiBlock then applies digging damage until the obstacle is destroyed.
  void player_plant_interact();

  //! @brief Break the targetted Cmp::LootContainer with a tool via Events::DestroyLootContainerEvent (see
  //! LootSystem::on_destroy_loot_container_event)
  void check_player_smash_pot();

  //! @brief If player is carry suitable weapon did the action event occur in NPC vicinity?
  void check_player_axe_npc_kill();

  //! @brief Uses Cmp::SelectedPosition to mark the targetted Cmp::Obstacle/Cmp::Moveable entity.
  void select_moveable_obstacle();

  //! @brief Removes all Cmp::SelectedPosition components
  void reset_all_selected_positions();

  //! @brief True if the player's last dig action is still within its cooldown window
  bool is_digging_on_cooldown();

  //! @brief Attempt to burn a plant
  void try_burn_plant();

  //! @brief Update the plant burning in progress
  //! @param dt
  void update_burning_worlditems( sf::Time dt );

  //! @brief Destroy the NPCs (Cmp::Npc::SpawnedBy) a plant spawned, e.g. when it burns out or is chopped down.
  //! @param plant_uuid The plant's Cmp::UUID.
  //! @param death_anim The death animation played for each NPC.
  void destroy_plant_npcs( const Cmp::UUID &plant_uuid, const Sys::SpriteKey &death_anim );

  //! @brief Destroy the uncollected loot (Cmp::DroppedBy) a plant dropped, e.g. when it burns out.
  //! @param plant_uuid The plant's Cmp::UUID.
  void destroy_plant_loot( const Cmp::UUID &plant_uuid );

  //! @brief Find a random free grid cell around the plant to drop a foraged item into. See Utils::is_free_spawn_area().
  //! @param plant_mb_cmp The plant being foraged.
  //! @return sf::Vector2f The free cell, or the plant's head cell if every surrounding cell is occupied.
  sf::Vector2f find_forage_drop_pos( const Cmp::PlantMultiBlock &plant_mb_cmp );

  //! @brief all grid positions that block NPC pathfinding
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief All grid positions that block ghost NPC pathfinding
  PathFinding::SpatialHashGridWeakPtr m_ghost_navmesh;

  //! @brief All grid positions that block player movement
  PathFinding::SpatialHashGridWeakPtr m_player_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;

  //! @brief Loot drops, so foraged items don't drop on them.
  PathFinding::SpatialHashGridWeakPtr m_loot_sm;
};
} // namespace Game::Sys

#endif // SRC_SYSTEMS_ACTIONSYSTEM_HPP__
