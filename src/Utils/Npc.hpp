#ifndef SRC_UTILS_NPC_HPP__
#define SRC_UTILS_NPC_HPP__

#include <Sprites/SpriteMetaType.hpp>
#include <source_location>
namespace Game::Cmp
{
class Position;
}

namespace Game::PathFinding
{
class SpatialHashGrid;
}

namespace Game::Utils::Npc
{

//! @brief Find the world entity (excluding player/NPC/footstep entities) whose position intersects `npc_pos`.
//! @param reg reference to the entt registry
//! @param npc_pos The position to test for intersection.
//! @return entt::entity The matching world entity, or entt::null if none intersects.
entt::entity get_world_pos_entt( entt::registry &reg, Cmp::Position npc_pos );

//! @brief Find the world entity (excluding player/NPC/footstep entities) whose position intersects
//! `npc_entt`'s own position.
//! @param reg reference to the entt registry
//! @param npc_entt The NPC entity whose Cmp::Position is used for the intersection test.
//! @return entt::entity The matching world entity, or entt::null if `npc_entt` has no Cmp::Position or none intersects.
entt::entity get_world_pos_entt( entt::registry &reg, entt::entity npc_entt );

//! @brief Get an NPC's sprite meta type from its Cmp::AnimData component.
//! @param reg reference to the entt registry
//! @param npc_entt The NPC entity to query.
//! @param loc Caller location, used in the thrown error message (defaults to the call site).
//! @return Sprites::SpriteMetaType The NPC's sprite type.
//! @throws std::runtime_error if `npc_entt` has no Cmp::AnimData component.
Sprites::SpriteMetaType get_sprite_type( entt::registry &reg, entt::entity npc_entt, std::source_location loc = std::source_location::current() );

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
//! @return PathfindResult Whether the NPC moved, was blocked, or no path was found.
PathfindResult pathfind_toward( entt::registry &reg, PathFinding::SpatialHashGrid &navmesh, const Cmp::Position &target_pos, entt::entity npc_entity,
                                bool target_in_spawn, bool target_illuminated, bool always_pathfind = false );

} // namespace Game::Utils::Npc

#endif // SRC_UTILS_NPC_HPP__
