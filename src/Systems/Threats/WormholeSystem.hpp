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
  //! @brief Construct a new Wormhole System object
  //! @param reg
  //! @param window
  //! @param sprite_factory
  //! @param sound_bank
  WormholeSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointer for the pathfinding navmesh
  //! @param npc_navmesh
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh, const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr )
  {
    m_npc_navmesh = npc_navmesh;
    m_reserved_sm = reserved_sm;
  }

  //! @brief event handlers for pausing system clocks
  void on_pause() override;
  //! @brief event handlers for resuming system clocks
  void on_resume() override;

  //! @brief Distinguishes the initial wormhole spawn from later respawns, controlling which position-picking strategy spawn_wormhole() uses.
  enum class SpawnPhase
  {
    //! @brief First spawn of the scene: uses the persisted seeded position.
    InitialSpawn,
    //! @brief Subsequent spawn after a despawn: picks a fresh random position.
    Respawn
  };

  //! @brief Find a valid spawn position, remove any obstacle/loot/NPC container occupying it, and
  //! attach the wormhole's Singularity/MultiBlock/AnimData components there.
  //! @param phase InitialSpawn uses the persisted seeded position; Respawn picks a fresh random one
  void spawn_wormhole( SpawnPhase phase );

  //! @brief Search for a valid, unoccupied position to spawn the wormhole, retrying with an
  //! incrementing seed until one is found that doesn't collide with walls, graves, altars, crypts,
  //! the graveyard exit, or hazard cells.
  //! @param seed Starting seed to search from; 0 means unseeded/random
  //! @return The chosen entity and its position, or {entt::null, {}} if no valid location was found
  //!         after the maximum number of attempts
  std::pair<entt::entity, Cmp::Position> find_spawn_location( unsigned long seed );

  //! @brief Track actors colliding with the wormhole via Cmp::Wormhole::Jump, drop the component if
  //! an actor stops colliding before its jump cooldown completes, and once every jump candidate's
  //! cooldown has elapsed, teleport them all to new random locations and respawn the wormhole.
  void check_player_wormhole_collision();

  //! @brief Remove the wormhole's Singularity and MultiBlock components (and its AnimData), effectively despawning it.
  void despawn_wormhole();

private:
  //! @brief Used for teleported entities to be re-inserted into the pathfinding navmesh.
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_WORMHOLESYSTEM_HPP__
