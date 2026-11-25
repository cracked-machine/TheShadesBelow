#ifndef SRC_UTILS_NPC_HPP__
#define SRC_UTILS_NPC_HPP__

#include <Systems/Stores/StoreKey.hpp>
#include <optional>
#include <source_location>

// clang-format off
namespace Game::Cmp::Npc { class Footprint; }
// clang-format on
namespace Game::Cmp
{
class Position;
struct UUID;
}

namespace Game::PathFinding
{
class SpatialHashGrid;
}

namespace Game::Utils::Collision
{
struct LightSources;
}

namespace Game::Utils::Npc
{

//! @brief Count the live NPCs spawned by a world item (see Cmp::Npc::SpawnedBy and Cmp::SpawnNpcEffect::max_instances).
//! @param reg reference to the entt registry
//! @param source_uuid The Cmp::UUID of the spawning world item.
//! @return int The number of NPCs whose Cmp::Npc::SpawnedBy matches `source_uuid`.
int count_spawned_by( entt::registry &reg, const Cmp::UUID &source_uuid );

//! @brief Get an NPC's sprite meta type from its Cmp::AnimData component.
//! @param reg reference to the entt registry
//! @param npc_entt The NPC entity to query.
//! @param loc Caller location, used in the thrown error message (defaults to the call site).
//! @return Sys::SpriteKey The NPC's sprite type.
//! @throws std::runtime_error if `npc_entt` has no Cmp::AnimData component.
Sys::SpriteKey get_sprite_type( entt::registry &reg, entt::entity npc_entt, std::source_location loc = std::source_location::current() );

//! @brief Get the world rect of an NPC that collides with the player: its footprint, since the rows of a multiblock
//! NPC above the cells it stands on are off the ground. NPCs without a Cmp::Npc::Footprint collide with their
//! whole Cmp::Position.
//! @param reg reference to the entt registry
//! @param npc_entt An NPC entity with a Cmp::Position.
//! @return The collision rect, or std::nullopt if the NPC's footprint does not collide (an all-false collision mask).
std::optional<sf::FloatRect> collision_bounds( entt::registry &reg, entt::entity npc_entt );

//! @brief Whether any cell `npc_entity` would stand on at `next_position` is taken by another NPC.
//! A mid-lerp NPC takes the cells it is heading to, not the ones it is leaving. NPCs whose footprint
//! doesn't collide never take a cell.
//! @param reg reference to the entt registry
//! @param npc_entity The NPC that would move.
//! @param footprint The moving NPC's footprint.
//! @param next_position The moving NPC's top-left world position after the move.
//! @return bool true if another NPC holds any of those cells.
bool next_cells_occupied( entt::registry &reg, entt::entity npc_entity, const Cmp::Npc::Footprint &footprint, sf::Vector2f next_position );

//! @brief Outcome of a single pathfind_toward() step
enum class PathfindResult {
  //! @brief A path was found and the NPC's Cmp::Direction/Cmp::LerpPosition were updated.
  Moved,
  //! @brief The NPC is off-screen, mid-lerp, or would cross the spawn boundary — no A* was run or the result was discarded.
  Blocked,
  //! @brief A* found no usable path; caller decides how to react (e.g. zero direction, pick a new target).
  NoPath,
};

//! @brief Pathfind a single NPC entity one grid step towards `target_pos` and, on success, update its
//! Cmp::Direction/Cmp::LerpPosition. Shared by NpcSystem (player-chasing NPCs) and WispSystem (target-chasing wisps).
//! @param reg reference to the entt registry
//! @param navmesh The spatial hash grid used by A* to find a path.
//! @param target_pos The world position the NPC is trying to reach.
//! @param npc_entity The NPC entity to move.
//! @param target_in_spawn true if `target_pos` is within the spawn area, used to stop the NPC at the boundary
//! @param target_illuminated true if the target (the player) is currently lit by a light source, used to stop
//! the NPC at the boundary of whichever light source it's about to step into
//! @param always_pathfind skip the "only pathfind when on-screen" check — wisps must always pathfind
//! @param lights Optional light snapshot. When given, lit cells are treated as obstacles (except lights illuminating
//! the target), so a light placed across a passage cuts the NPC off from the player.
//! @param closest_if_unreachable If the target can't be reached, step towards the closest reachable cell instead of
//! returning PathfindResult::NoPath (which is still returned once the NPC is on that cell).
//! @return PathfindResult Whether the NPC moved, was blocked, or no path was found.
PathfindResult pathfind_toward( entt::registry &reg, PathFinding::SpatialHashGrid &navmesh, const Cmp::Position &target_pos, entt::entity npc_entity,
                                bool target_in_spawn, bool target_illuminated, bool always_pathfind = false,
                                const Utils::Collision::LightSources *lights = nullptr, bool closest_if_unreachable = false );

} // namespace Game::Utils::Npc

#endif // SRC_UTILS_NPC_HPP__
