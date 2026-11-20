#ifndef SRC_SYSTEMS_THREATS_HAZARDFIELDSYSTEM_HPP__
#define SRC_SYSTEMS_THREATS_HAZARDFIELDSYSTEM_HPP__

#include <Components/Hazard/CorruptionCell.hpp>
#include <Components/Hazard/FieldCell.hpp>
#include <Components/Hazard/SinkholeCell.hpp>
#include <Components/Persistent/CorruptionDamage.hpp>
#include <Components/Persistent/CorruptionSeed.hpp>
#include <Components/Persistent/SinkholeSeed.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Position.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/BaseSystem.hpp>
#include <Utils/Random.hpp>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <entt/entity/fwd.hpp>

#include <span>
#include <vector>

//! @brief HazardFields are environmental dangers that spread throughout the maze,
//! posing threats to both the player and NPCs. Examples include sinkholes (composed of many
//! Cmp::SinkholeCell) that instantly kill any entity that falls into them, and corruption fields
//! (composed of many Cmp::CorruptionCell) that gradually drain health over time.

namespace Game::Sys
{

template <typename T>
struct HazardTraits;

//! @brief Component requirements for a hazard field cell type: must provide a HazardTraits<T> specialization defining its seed component,
//! mutually-exclusive hazard type, instant-kill flag, mortality state and sprite type.
template <typename T>
concept ValidHazard = requires {
  typename HazardTraits<T>::SeedType;
  typename HazardTraits<T>::ExcludeHazard;
  { HazardTraits<T>::kills_instantly } -> std::convertible_to<bool>;
  { HazardTraits<T>::mortality_state } -> std::convertible_to<Cmp::Player::Mortality::State>;
  { HazardTraits<T>::sprite_type } -> std::convertible_to<std::string_view>;
  { HazardTraits<T>::spreads_to_open_floor } -> std::convertible_to<bool>;
};

//! @brief A templated system that manages the creation and spread of hazard
//! fields in a procedural maze.
//! @tparam HazardType The type of hazard component that this system will manage.
//! Must satisfy the ValidHazard concept.
template <ValidHazard HazardType>
class HazardFieldSystem : public Sys::BaseSystem
{
  //! @brief Convenience alias for the HazardTraits<HazardType> specialization backing this instantiation.
  using Traits = HazardTraits<HazardType>;

public:
  //! @brief Construct a new Hazard Field System object
  //!
  //! @param reg Smart pointer to the entt registry
  //! @param window Reference to the SFML render window
  HazardFieldSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointers for the reserved-positions grid and the NPC navmesh.
  //! @param reserved_sm Positions the field must never spread into.
  //! @param npc_navmesh NPC pathfinding navmesh; cells are removed from it as the field spreads over open floor.
  //! @param world_tile_sm World-tile lookup (see Scene::m_world_tile_sm) used to find spread candidates around each
  //! hazard cell; null falls back to scanning candidate_view().
  //! @param loot_sm Loot drops (Cmp::Loot); any on a cell the field takes over are destroyed.
  //! @param item_sm Ground world items (see Scene::m_item_sm).
  //! @param hazard_sm Sinkhole/corruption cells (see Scene::m_hazard_sm).
  void init( const PathFinding::SpatialHashGridSharedPtr &reserved_sm, const PathFinding::SpatialHashGridSharedPtr &npc_navmesh,
             const PathFinding::SpatialHashGridSharedPtr &world_tile_sm = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &loot_sm = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &item_sm = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &hazard_sm = nullptr )
  {
    m_reserved_sm = reserved_sm;
    m_npc_navmesh = npc_navmesh;
    m_world_tile_sm = world_tile_sm;
    m_loot_sm = loot_sm;
    m_item_sm = item_sm;
    m_hazard_sm = hazard_sm;
  }

  //! @brief Spread the hazard field, then check for player and NPC collisions with it.
  //! @return The position of any newly added hazard cell, or an empty vector if none was added.
  sf::Vector2f update( sf::Time dt );

  //! @brief Seed one hazard field per candidate set, unless this hazard type already has cells (e.g. re-entering the scene).
  //! Each field starts at a random position from its set that has a world entity, is not a wall, exit or other hazard,
  //! and is not reserved. If HazardTraits::SeedType is non-zero the choice is deterministic.
  //! @param candidate_sets One list of candidate pixel positions per field, e.g. the open tiles of one map quadrant.
  //! @return The positions of the newly seeded hazard cells; empty if cells already existed.
  std::vector<sf::Vector2f> init_hazard_field( std::span<const std::vector<sf::Vector2f>> candidate_sets );

private:
  //! @brief Update the hazard field by spreading it to adjacent positions.
  //! 1. Accumulate dt and check if the update interval has elapsed.
  //! 2. Get a view of all entities with the hazard field component and position.
  //! 3. For each hazard field entity:
  //!   a. Check adjacent positions for candidate cells (obstacles, plus open floor if
  //!   HazardTraits::spreads_to_open_floor) that are not reserved and not already hazard cells.
  //!   b. Use a random 1-in-HazardTraits::odds chance to convert an adjacent candidate into a new hazard cell.
  //!   c. Count adjacent hazard fields; if surrounded by 2 or more, mark as inactive to stop
  //!   further spreading.
  //! @param dt Frame delta, accumulated into m_spread_update_timer
  sf::Vector2f update_hazard_field( sf::Time dt );

  //! @brief View of the world tiles (Cmp::WorldTile) a field may start on or spread into, before the
  //! reserved-positions check: excludes obstacle caps, walls, loot/NPC containers and existing hazard cells.
  auto candidate_view();

  //! @brief Whether `entity` would appear in candidate_view(), for entities found via m_world_tile_sm instead of the view.
  //! @param entity Any entity, possibly invalid.
  bool in_candidate_view( entt::entity entity );

  //! @brief Whether a candidate_view() entity can become a hazard cell: an obstacle's main block, or open
  //! floor when HazardTraits::spreads_to_open_floor.
  //! @param entity Entity from candidate_view().
  bool is_candidate( entt::entity entity );

  //! @brief Whether a quest key (exit/crypt key) lies on this cell. Keys block the field so a level can't become unwinnable.
  //! @param pos The cell to test.
  bool holds_key_item( const Cmp::Position &pos );

  //! @brief Check for player collision with hazard fields.
  //! If a collision is detected, apply damage or instant death based on hazard type.
  void check_player_hazard_field_collision();
  //! @brief Check for NPC collision with hazard fields.
  //! If a collision is detected, trigger NPC death event.
  void check_npc_hazard_field_collision();

  //! @brief Convert an obstacle or open floor entity into an active hazard cell: transfers its spatial-hash reservation,
  //! removes the cell from the NPC navmesh and attaches the hazard/anim/z-order/pathfinding components.
  //! @param entity The obstacle/floor entity to promote.
  //! @param pos The entity's current position. Taken by value: remove_obstacle() destroys cap entities, which can
  //! swap-and-pop the Cmp::Position storage a caller's reference points into.
  //! @param reserved_sm The locked reserved-positions grid, or nullptr if it has expired.
  void promote_to_hazard_cell( entt::entity entity, const Cmp::Position pos, const PathFinding::SpatialHashGridSharedPtr &reserved_sm );

  //! @brief Clear m_field_sm and repopulate it from every HazardType entity currently in the registry.
  void rebuild_field_sm();

  //! @brief Check whether m_field_sm holds a valid HazardType entity at `pos` + `offset`.
  //! @param pos The reference cell position.
  //! @param offset Pixel offset from `pos` of the cell to check.
  //! @return true if that cell is part of this hazard field.
  bool has_hazard_at( const Cmp::Position &pos, sf::Vector2f offset ) const;

  //! @brief Pick the sinkhole edge sprite for `entity` from which of its 4 cardinal neighbours are also
  //! hazard cells (looked up in m_field_sm), and swap its AnimData sprite to match. Also sets/removes
  //! Cmp::Hazard::SinkholeCorners for any inner corners (both adjacent sides open, diagonal not).
  //! @param entity The hazard cell entity to re-sprite.
  //! @param pos The entity's position.
  void update_sinkhole_sprite( entt::entity entity, const Cmp::Position &pos );

  //! @brief Re-sprite a newly added sinkhole cell and each of its 8 surrounding sinkhole neighbours so the
  //! shared edges open up and the field reads as one continuous sinkhole.
  //! @param entity The newly added hazard cell entity.
  //! @param pos The entity's position.
  void retile_sinkhole_cell_and_neighbours( entt::entity entity, const Cmp::Position &pos );

  //! @brief Gameplay time elapsed since the last hazard field update.
  //!
  sf::Time m_spread_update_timer{ sf::Time::Zero };

  sf::Time m_dmg_timer{ sf::Time::Zero };

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;

  //! @brief NPC pathfinding navmesh, so NPCs stop routing over cells the field spreads into.
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief World-tile lookup, so spreading only checks the 3x3 cells around each hazard cell.
  PathFinding::SpatialHashGridWeakPtr m_world_tile_sm;

  //! @brief Loot drops, destroyed when the field spreads onto their cell.
  PathFinding::SpatialHashGridWeakPtr m_loot_sm;

  //! @brief Ground world items (see Scene::m_item_sm).
  PathFinding::SpatialHashGridWeakPtr m_item_sm;

  //! @brief Sinkhole/corruption cells (see Scene::m_hazard_sm).
  PathFinding::SpatialHashGridWeakPtr m_hazard_sm;

  //! @brief Spatial index of this system's hazard cells (one bucket per grid cell), used to track the
  //! structure of the field as it spreads.
  PathFinding::SpatialHashGrid m_field_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_HAZARDFIELDSYSTEM_HPP__
