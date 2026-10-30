#ifndef SRC_SYSTEMS_THREATS_WORMHOLESYSTEM_HPP__
#define SRC_SYSTEMS_THREATS_WORMHOLESYSTEM_HPP__

#include <Components/Position.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <entt/entity/fwd.hpp>

#include <Systems/BaseSystem.hpp>

namespace Game::Sys
{

//! @brief Drives the wormhole hazard's lifecycle: spawns a wormhole at a random valid location,
//! teleports the player (and other colliding actors) to a new random location on collision, then
//! despawns and respawns the wormhole. Cycle: spawn -> player collision -> teleport -> despawn -> repeat.
class WormholeSystem : public BaseSystem
{
public:
  //! @brief Distinguishes the initial wormhole spawn from later respawns, controlling which position-picking strategy spawn_wormhole() uses.
  enum class SpawnPhase {
    //! @brief First spawn of the scene: uses the persisted seeded position.
    InitialSpawn,
    //! @brief Subsequent spawn after a despawn: picks a fresh random position.
    Respawn
  };

  //! @brief Construct a new Wormhole System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  WormholeSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointer for the pathfinding navmesh
  //! @param npc_navmesh
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh, const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr )
  {
    m_npc_navmesh = npc_navmesh;
    m_reserved_sm = reserved_sm;
  }

  //! @brief Track actors colliding with the wormhole via Cmp::PendingTeleport, drop the component if
  //! an actor stops colliding before its countdown completes, and once every jump candidate's
  //! countdown has elapsed, teleport them all to new random locations and respawn the wormhole.
  //! @note TeleportSystem ticks the countdowns and fades the candidates out. A Cmp::PendingTeleport holding an
  //! event belongs to another (delayed) teleport and is ignored here.
  void check_player_wormhole_collision();

  //! @brief Time since the current jump sequence started (the oldest wormhole Cmp::PendingTeleport timer), zero if none.
  //! @param reg
  //! @return sf::Time
  static sf::Time jump_sequence_elapsed( entt::registry &reg );

  //! @brief Whether the player is a jump candidate and the sequence has passed Cmp::Persist::WormholeAbortThreshold.
  //! The player is then committed to the teleport and cannot take off. See Sys::FlyingSystem.
  //! @param reg
  //! @return bool
  static bool is_player_committed_to_jump( entt::registry &reg );

  //! @brief Find a valid spawn position, remove any obstacle/loot/NPC container occupying it, and
  //! attach the wormhole's Singularity/MultiBlock/AnimData components there.
  //! @param phase InitialSpawn uses the persisted seeded position; Respawn picks a fresh random one
  void spawn_wormhole( SpawnPhase phase );

private:
  //! @brief A player flying over the singularity aborts the whole jump sequence (all candidates, no teleport,
  //! no wormhole respawn) unless it has passed Cmp::Persist::WormholeAbortThreshold.
  //! @return bool true if the player is flying over the singularity before the threshold - skip this frame's jump processing.
  bool abort_jump_if_flying_over();

  //! @brief Remove all entities within the `bounds` and replace with new Position/Armable entities.
  //! @param bounds The world-space rect to sweep for occupying entities.
  void clear_footprint( const sf::FloatRect &bounds );

  //! @brief Remove the wormhole's Singularity and MultiBlock components (and its AnimData), effectively despawning it.
  void despawn_wormhole();

  //! @brief Search for a valid, unoccupied position to spawn the wormhole, retrying with an
  //! incrementing seed until one is found that doesn't collide with walls, graves, altars, crypts,
  //! the graveyard exit, or hazard cells.
  //! @param initial_rng_seed Starting seed to search from; 0 means unseeded/random
  //! @return The chosen entity and its position, or {entt::null, {}} if no valid location was found
  //!         after the maximum number of attempts
  std::pair<entt::entity, Cmp::Position> find_spawn_location( unsigned long initial_rng_seed );

  //! @brief Used for teleported entities to be re-inserted into the pathfinding navmesh.
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_WORMHOLESYSTEM_HPP__
