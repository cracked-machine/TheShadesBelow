#ifndef SRC_SYSTEMS_PLANTSYSTEM_HPP__
#define SRC_SYSTEMS_PLANTSYSTEM_HPP__

#include <Events/CreatePlantEvent.hpp>
#include <Events/DestroyPlantEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/StoreKey.hpp>

namespace Game::Cmp
{
class PlantMultiBlock;
struct UUID;
} // namespace Game::Cmp

namespace Game::Sys
{

//! @brief Handles plant creation and removal (Events::CreatePlantEvent, Events::DestroyPlantEvent), player plant
//! interactions (foraging, chopping, digging up and burning, via Events::PlayerActionEvent) and burning plants.
class PlantSystem : public BaseSystem
{
public:
  //! @brief Construct a new Plant System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  PlantSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointers for the pathfinding navmeshes. reserved_sm is first so it can be set before level
  //! generation, when the navmeshes don't exist yet.
  //! @param reserved_sm
  //! @param npc_navmesh
  //! @param player_navmesh
  //! @param ghost_navmesh
  //! @param loot_sm loot drops, so foraged items don't drop on them
  void init( const PathFinding::SpatialHashGridSharedPtr &reserved_sm, const PathFinding::SpatialHashGridSharedPtr &npc_navmesh = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &player_navmesh = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &ghost_navmesh = nullptr, const PathFinding::SpatialHashGridSharedPtr &loot_sm = nullptr )
  {
    m_reserved_sm = reserved_sm;
    m_npc_navmesh = npc_navmesh;
    m_player_navmesh = player_navmesh;
    m_ghost_navmesh = ghost_navmesh;
    m_loot_sm = loot_sm;
  }

  //! @brief frame updates
  //! @param dt
  void update( sf::Time dt );

private:
  //! @brief Event handler for player actions
  //! @param event
  void on_player_action( const Events::PlayerActionEvent &event );

  //! @brief Create the plant if its footprint is clear. See gen_plant().
  //! @param event
  void on_create_plant_event( const Events::CreatePlantEvent &event );

  //! @brief Remove the plant from the navmeshes, then destroy it. This is the only place allowed to remove plants;
  //! everything else must trigger an Events::DestroyPlantEvent.
  //! @param event
  void on_destroy_plant_event( const Events::DestroyPlantEvent &event );

  //! @brief Generate the given plant type in the game area.
  //! @param plant_type The plant world item, e.g. "item.plant.nettle"
  //! @param pos_cmp Position to place the plant
  //! @return entt::entity The new plant, or entt::null if its footprint isn't clear.
  entt::entity gen_plant( const Sys::ItemKey &plant_type, const Cmp::Position &pos_cmp );

  //! @brief Uses Cmp::SelectedPosition to mark the targetted Cmp::PlantMultiBlock then applies digging damage until the obstacle is destroyed.
  void player_plant_interact();

  //! @brief Attempt to burn a plant
  void try_burn_plant();

  //! @brief Update the plant burning in progress
  //! @param dt
  void update_burning_plants( sf::Time dt );

  //! @brief Destroy the NPCs (Cmp::Npc::SpawnedBy) a plant spawned, e.g. when it burns out or is chopped down.
  //! @param plant_uuid The plant's Cmp::UUID.
  //! @param death_anim The death animation played for each NPC.
  void destroy_plant_spawned_npcs( const Cmp::UUID &plant_uuid, const Sys::SpriteKey &death_anim );

  //! @brief Destroy the uncollected loot (Cmp::DroppedBy) a plant dropped, e.g. when it burns out.
  //! @param plant_uuid The plant's Cmp::UUID.
  void destroy_plant_spawned_loot( const Cmp::UUID &plant_uuid );

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

#endif // SRC_SYSTEMS_PLANTSYSTEM_HPP__
