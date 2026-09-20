#ifndef SRC_SYSTEMS_THREATS_NPCSYSTEM_HPP__
#define SRC_SYSTEMS_THREATS_NPCSYSTEM_HPP__

#include <Systems/BaseSystem.hpp>

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include <entt/entity/fwd.hpp>

// clang-format off
namespace Game::Sprites { class SpriteFactory; }
namespace Game::Sys { class Store; }
namespace Game::Audio { class SoundBank; }
namespace Game::Cmp { class Position; }
namespace Game::Cmp { class Direction; class LerpPosition; } 
namespace Game::Cmp::Player { class Mortality; } 
namespace Game::Cmp::Npc { class NPC; class Shockwave; }
namespace Game::PathFinding { class SpatialHashGrid; }
// clang-format on
namespace Game::Sys
{

//! @brief Drives NPC movement, pathfinding, animation, sound effects, and collision handling
//! (container proximity, timed/instant collision actions, knockback) for all NPCs in the scene.
class NpcSystem : public BaseSystem
{
public:
  //! @brief Construct a new Npc System object
  //! @param reg
  //! @param window
  //! @param sprite_factory
  //! @param sound_bank
  NpcSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointer for the pathfinding navmesh
  //! @param npc_navmesh
  //! @param open_navmesh
  //! @param ghost_navmesh variant that ignores plant segments; ghosts fall back to npc_navmesh when unset
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh, const PathFinding::SpatialHashGridSharedPtr &open_navmesh,
             const PathFinding::SpatialHashGridSharedPtr &ghost_navmesh = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr )
  {
    m_npc_navmesh = npc_navmesh;
    m_open_navmesh = open_navmesh;
    m_ghost_navmesh = ghost_navmesh;
    m_reserved_sm = reserved_sm;
  }

  //! @brief Update the NpcSystem
  //! @param dt Delta time since last update call
  void update( sf::Time dt );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Pick the navmesh matching an NPC's traversal type: wisps roam the open
  //! navmesh, ghosts use the plant-permeable ghost navmesh, everyone else the npc navmesh
  //! @param npc_entity
  PathFinding::SpatialHashGridSharedPtr navmesh_for( entt::entity npc_entity );

  //! @brief Updates lerp movement for all NPCs
  //! @param dt
  void update_movement( sf::Time dt );

  //! @brief Update lerp movement for specific NPC entity and navmesh
  //! @param navmesh
  //! @param npc_entity
  //! @param dt
  void update_movement_for( PathFinding::SpatialHashGrid &navmesh, entt::entity npc_entity, sf::Time dt );

  //! @brief Update the NPC sfx
  void update_sfx();

  //! @brief Check for player collision proximity with NPC containers
  void check_npc_container_collision();

  //! @brief Check Player/NPC collision for Cmp::CollisionActions with zero tick.
  //! @note Player cooldown and knockback are ENABLED.
  void check_once_collision();

  //! @brief Check Player/NPC collision for Cmp::CollisionActions with non-zero tick.
  //! @note Player cooldown and knockback are DISABLED.
  void check_timed_collision( sf::Time dt );

  //! @brief Find a suitable knockback position for the player
  //! @param npc_direction
  void find_pushback_position( const Cmp::Direction &npc_direction );

  //! @brief Transition the player to HAUNTED and enqueue the mortality event if health has depleted.
  //! @param player_mort
  //! @return true if the player died
  bool check_player_death( Cmp::Player::Mortality &player_mort );

  //! @brief Update pathfinding for all NPCs.
  //! @param dt
  void update_pathfinding( sf::Time dt );

  //! @brief Update pathfinding for specific NPC entities and navmesh.
  //! @param navmesh
  //! @param target_pos
  //! @param npc_entity
  //! @param target_in_spawn
  void update_pathfinding_for( PathFinding::SpatialHashGrid &navmesh, const Cmp::Position &target_pos, entt::entity npc_entity,
                               bool target_in_spawn );

  //! @brief Update NPC animation
  void update_animation();

  //! @brief track time since last pathfinding update
  sf::Time m_pathfinding_timer;

  //! @brief Track time since last animation frame
  sf::Time m_anim_timer;

  //! @brief Track time since has container activiation check
  sf::Time m_container_timer;

  //! @brief Used for NPCs to pathfind around blocking obstacles
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief Used for ghost NPCs to pathfind while ignoring plant segments; falls back to m_npc_navmesh when unset.
  PathFinding::SpatialHashGridWeakPtr m_ghost_navmesh;

  //! @brief Used for NPCs to move freely within the level boundaries.
  PathFinding::SpatialHashGridWeakPtr m_open_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_NPCSYSTEM_HPP__
