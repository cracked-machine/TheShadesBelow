#ifndef SRC_SYSTEMS_THREATS_NPCSYSTEM_HPP__
#define SRC_SYSTEMS_THREATS_NPCSYSTEM_HPP__

#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include <entt/entity/fwd.hpp>

#include <optional>

// clang-format off
namespace Game::Sys { class Store; }
namespace Game::Audio { class SoundBank; }
namespace Game::Events { class UpdateNpcPositionEvent; class DestroyNpcEvent; struct CreateNpcEvent; }
namespace Game::Cmp { class Position; }
namespace Game::Cmp { class Direction; class LerpPosition; } 
namespace Game::Cmp::Player { class Mortality; } 
namespace Game::Cmp::Npc { class NPC; class Shockwave; }
namespace Game::PathFinding { class SpatialHashGrid; }
namespace Game::Utils::Collision { struct LightSources; }
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
  //! @param sound_bank
  NpcSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointer for the pathfinding navmesh
  //! @param npc_navmesh
  //! @param open_navmesh
  //! @param ghost_navmesh variant that ignores plant segments; ghosts fall back to npc_navmesh when unset
  //! @param reserved_sm
  //! @param loot_sm loot drops, so NPCs don't spawn on them
  void init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh, const PathFinding::SpatialHashGridSharedPtr &open_navmesh,
             const PathFinding::SpatialHashGridSharedPtr &ghost_navmesh = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr, const PathFinding::SpatialHashGridSharedPtr &loot_sm = nullptr )
  {
    m_npc_navmesh = npc_navmesh;
    m_open_navmesh = open_navmesh;
    m_ghost_navmesh = ghost_navmesh;
    m_reserved_sm = reserved_sm;
    m_loot_sm = loot_sm;
  }

  //! @brief Update the NpcSystem
  //! @param dt Delta time since last update call
  void update( sf::Time dt );

private:
  //! @brief Relocate an NPC and keep its navmesh in sync. This is the only place allowed to write an NPC's
  //! Cmp::Position outside of its own lerp movement. Everything else must trigger an Events::UpdateNpcPositionEvent.
  //! @param event
  void on_update_npc_position_event( const Game::Events::UpdateNpcPositionEvent &event );

  //! @brief Kill an NPC and keep its navmesh in sync, optionally playing a death animation where it stood. This is
  //! the only place allowed to call Factory::Npc::destroy_npc. Everything else must trigger an Events::DestroyNpcEvent.
  //! @param event
  void on_destroy_npc_event( const Game::Events::DestroyNpcEvent &event );

  //! @brief Spawn an NPC, unless its source item already has `max_instances` live NPCs. This is the only place
  //! allowed to call Factory::Npc::create_npc. Everything else must trigger an Events::CreateNpcEvent.
  //! @param event
  void on_create_npc_event( const Game::Events::CreateNpcEvent &event );

  //! @brief Find a random free cell for `npc_type` in the nearest ring around `source` that has one,
  //! searching outwards up to kNpcSpawnSearchRadius rings.
  //! @param source The spawning source's bounds, e.g. a plant's whole footprint.
  //! @param npc_type See res/json/npc.json.
  //! @return std::optional<sf::Vector2f> top-left of the cell, or nullopt if none was found.
  std::optional<sf::Vector2f> find_nearby_spawn_cell( const Cmp::Position &source, const Sys::NpcKey &npc_type );

  //! @brief How many rings out from the source find_nearby_spawn_cell() searches.
  static constexpr int kNpcSpawnSearchRadius = 6;

  //! @brief Create the death animation entity played when an NPC dies.
  //! @param npc_pos_cmp The dying NPC's position.
  //! @param death_anim
  //! @return The new death animation entity.
  entt::entity create_death_anim( const Cmp::Position &npc_pos_cmp, const Sys::SpriteKey &death_anim );

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
  void update_sfx( sf::Time dt );

  //! @brief Check for player collision proximity with NPC containers
  void check_npc_container_collision();

  //! @brief Banish the ElderMother when she comes within range of a player carrying a usable ElderFlute.
  void check_elderflute_banish();

  //! @brief Whether the player is flying over an NPC too small (1x1) to reach them.
  //! @param npc_bounds The NPC's collision bounds.
  //! @return bool true if the NPC can't touch the flying player.
  bool is_flying_over( const sf::FloatRect &npc_bounds );

  //! @brief Check Player/NPC collision for Cmp::CollisionActions with zero tick.
  //! @note Player cooldown and knockback are ENABLED.
  void check_once_collision();

  //! @brief Check Player/NPC collision for Cmp::CollisionActions with non-zero tick.
  //! @note Player cooldown and knockback are DISABLED.
  void check_timed_collision( sf::Time dt );

  //! @brief Kill any NPC (except ghosts) hit by a burning particle: any sprite whose tag contains
  //! Cmp::Particle::LavaFireballParticleSprite::kTagSuffix or Cmp::Particle::EmberParticleSprite::kTagSuffix.
  //! The particle is destroyed by the hit.
  void check_burning_particle_collision();

  //! @brief Find a suitable knockback position for the player
  //! @param npc_direction
  //! @param npc_bounds The NPC's collision bounds; the player is pushed clear of all of it.
  void find_pushback_position( const Cmp::Direction &npc_direction, const sf::FloatRect &npc_bounds );

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
  //! @param target_illuminated
  //! @param lights light snapshot for this tick; lights not covering the target or NPC block the path
  void update_pathfinding_for( PathFinding::SpatialHashGrid &navmesh, const Cmp::Position &target_pos, entt::entity npc_entity,
                               bool target_illuminated, const Utils::Collision::LightSources &lights );

  //! @brief Get the flee target for an NPC while the player has Cmp::Player::RepelNpc: the point one cell outside
  //!        `view_bounds` on the ray from the player through the NPC. Once the NPC leaves the view, pathfind_toward
  //!        stops it until the view moves.
  //! @param player_pos
  //! @param npc_pos
  //! @param view_bounds current world view bounds
  //! @return Cmp::Position the flee target, sized as `player_pos`
  Cmp::Position repel_target( const Cmp::Position &player_pos, const Cmp::Position &npc_pos, const sf::FloatRect &view_bounds ) const;

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

  //! @brief Loot drops, so NPCs don't spawn on them.
  PathFinding::SpatialHashGridWeakPtr m_loot_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_NPCSYSTEM_HPP__
