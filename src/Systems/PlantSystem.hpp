#ifndef SRC_SYSTEMS_PLANTSYSTEM_HPP__
#define SRC_SYSTEMS_PLANTSYSTEM_HPP__

#include <Events/CreatePlantEvent.hpp>
#include <Events/DestroyPlantEvent.hpp>
#include <Events/PlantKilledEvent.hpp>
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

  //! @brief init the weak pointers for the pathfinding navmeshes. reserved_sm and world_tile_sm are first so they can be
  //! set before level generation, when the navmeshes don't exist yet.
  //! @param reserved_sm
  //! @param world_tile_sm world-tile lookup (see Scene::m_world_tile_sm); null falls back to scanning the registry
  //! @param npc_navmesh
  //! @param player_navmesh
  //! @param ghost_navmesh
  //! @param loot_sm loot drops, so plant drops don't land on them
  //! @param item_sm Ground world items (see Scene::m_item_sm).
  //! @param hazard_sm Sinkhole/corruption cells (see Scene::m_hazard_sm).
  void init( const PathFinding::SpatialHashGridSharedPtr &reserved_sm, const PathFinding::SpatialHashGridSharedPtr &world_tile_sm = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &item_sm = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &hazard_sm = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &npc_navmesh = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &player_navmesh = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &ghost_navmesh = nullptr, const PathFinding::SpatialHashGridSharedPtr &loot_sm = nullptr )
  {
    m_reserved_sm = reserved_sm;
    m_world_tile_sm = world_tile_sm;
    m_npc_navmesh = npc_navmesh;
    m_player_navmesh = player_navmesh;
    m_ghost_navmesh = ghost_navmesh;
    m_loot_sm = loot_sm;
    m_item_sm = item_sm;
    m_hazard_sm = hazard_sm;
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

  //! @brief A player weapon hit (shovel or arrow, see Utils::Player::apply_weapon_hit) took the plant's health to zero:
  //! drop one random item of its destroy_action reward, apply its destroy_action and side effects, then remove it.
  //! @param event
  void on_plant_killed_event( const Events::PlantKilledEvent &event );

  //! @brief Generate the given plant type in the game area.
  //! @param plant_type The plant world item, e.g. "item.plant.nettle"
  //! @param pos_cmp Position to place the plant
  //! @return entt::entity The new plant, or entt::null if its footprint isn't clear.
  entt::entity gen_plant( const Sys::ItemKey &plant_type, const Cmp::Position &pos_cmp );

  //! @brief Uses Cmp::SelectedPosition to mark the mouse-selected Cmp::PlantMultiBlock the player is facing. With the shovel
  //! equipped, each hit takes its damage_from_weapon.shovel (items.json) from its health. At zero health the plant is
  //! killed, see on_plant_killed_event(). With an empty hand or gloves the plant is foraged instead, see forage_plant().
  void player_plant_interact();

  //! @brief Forage the plant: drop its forage_action reward beside it, apply its forage_action and side effects, and
  //! drain its health by Cmp::Persist::PlantWearPerForage. A fully foraged plant withers away with no drop.
  //! Gloves stop the forage's health damage (not its benefits) and the drop's sting, and wear by
  //! Cmp::Persist::GlovesWearPerForage. Burning plants and plants without a forage_action reward can't be foraged.
  //! @param plant_entt The Cmp::PlantMultiBlock entity, which must have a Cmp::WorldItem.
  //! @param plant_mb_cmp The plant's multiblock.
  //! @param wearing_gloves true if the player has gloves in hand.
  void forage_plant( entt::entity plant_entt, const Cmp::PlantMultiBlock &plant_mb_cmp, bool wearing_gloves );

  //! @brief Attempt to burn a plant
  void try_burn_plant();

  //! @brief Update the plant burning in progress
  //! @param dt
  void update_burning_plants( sf::Time dt );

  //! @brief Destroy the NPCs (Cmp::Npc::SpawnedBy) a plant spawned, e.g. when it burns out or is destroyed by the shovel.
  //! @param plant_uuid The plant's Cmp::UUID.
  //! @param death_anim The death animation played for each NPC.
  void destroy_plant_spawned_npcs( const Cmp::UUID &plant_uuid, const Sys::SpriteKey &death_anim );

  //! @brief Destroy the uncollected loot (Cmp::DroppedBy) a plant dropped, e.g. when it burns out.
  //! @param plant_uuid The plant's Cmp::UUID.
  void destroy_plant_spawned_loot( const Cmp::UUID &plant_uuid );

  //! @brief Find a random free grid cell around the plant to drop its destroy_action reward into. See Utils::is_free_spawn_area().
  //! @param plant_mb_cmp The plant being destroyed.
  //! @return sf::Vector2f The free cell, or the plant's head cell if every surrounding cell is occupied.
  sf::Vector2f find_drop_pos( const Cmp::PlantMultiBlock &plant_mb_cmp );

  //! @brief all grid positions that block NPC pathfinding
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief All grid positions that block ghost NPC pathfinding
  PathFinding::SpatialHashGridWeakPtr m_ghost_navmesh;

  //! @brief All grid positions that block player movement
  PathFinding::SpatialHashGridWeakPtr m_player_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;

  //! @brief Loot drops, so plant drops don't land on them.
  PathFinding::SpatialHashGridWeakPtr m_loot_sm;

  //! @brief World-tile lookup used by gen_plant() instead of scanning every Cmp::Position. See Scene::m_world_tile_sm.
  PathFinding::SpatialHashGridWeakPtr m_world_tile_sm;

  //! @brief Ground world items (see Scene::m_item_sm).
  PathFinding::SpatialHashGridWeakPtr m_item_sm;

  //! @brief Sinkhole/corruption cells (see Scene::m_hazard_sm).
  PathFinding::SpatialHashGridWeakPtr m_hazard_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_PLANTSYSTEM_HPP__
