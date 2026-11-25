#ifndef SRC_SYSTEMS_THREATS_WATCHMANSYSTEM_HPP__
#define SRC_SYSTEMS_THREATS_WATCHMANSYSTEM_HPP__

#include <Components/UUID.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <utility>
#include <vector>

// clang-format off
namespace Game::Cmp { class Position; }
// clang-format on

namespace Game::Sys
{

//! @brief Drives the Watchman NPC: rate-limited spawning, searchlight sweep/lock-on behaviour,
//! gunfire cocking/firing timing, and gunfire collision against the player and other NPCs (applied via Events::WeaponHitEvent).
class WatchmanSystem : public BaseSystem
{
public:
  //! @brief Construct a new Watchman System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  WatchmanSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Set the cells Watchmen may spawn on and the grids that veto a spawn cell.
  //! @param path_positions Levelgen 'path' cells (pixels), see SceneData::path_tile_id().
  //! @param item_sm Ground world items (see Scene::m_item_sm).
  //! @param hazard_sm Sinkhole/corruption cells (see Scene::m_hazard_sm).
  void init( std::vector<sf::Vector2f> path_positions, const PathFinding::SpatialHashGridSharedPtr &item_sm = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &hazard_sm = nullptr )
  {
    m_path_positions = std::move( path_positions );
    m_item_sm = item_sm;
    m_hazard_sm = hazard_sm;
  }

  //! @brief Rate-limit and trigger Watchman NPC spawning in the graveyard scene
  //! @param dt Delta time since last update call
  void update( sf::Time dt );

private:
  //! @brief Rate-limit the spawning of Watchman NPCs
  sf::Time m_watchman_spawn_timer;

  //! @brief Create a watchman NPC on a random path cell (see m_path_positions).
  void spawn_watchman();

  //! @brief Advance each Watchman's searchlight sweep phase and recompute its cone direction,
  //! centered on the NPC's current walking direction (falling back to the last known facing
  //! direction while stationary).
  void update_searchlights( sf::Time dt );

  //! @brief Per-Watchman fire-rate state machine, gated by whether the player is currently
  //! inside that Watchman's searchlight cone.
  void update_gunfire( sf::Time dt );

  //! @brief Test whether any Cmp::Player::NoPath blocker (obstacles, walls, and every
  //! solid multiblock segment — altars, graves, crypts, ruins, plants) crosses the line between
  //! `from` and `to`, so the searchlight can't see/catch the player through cover.
  bool has_line_of_sight( sf::Vector2f from, sf::Vector2f to );

  //! @brief Spawn the gunfire particle effect and play the firing sound effect.
  //! @param npc_pos_cmp
  //! @param npc_uuid_cmp
  void fire_gun( Cmp::Position &npc_pos_cmp, Cmp::UUID &npc_uuid_cmp );

  //! @brief Play the gun-cocking/reload sound effect ahead of the next shot.
  //! @param npc_pos_cmp
  void cock_gun( Cmp::Position &npc_pos_cmp );

  //! @brief Hit the player (Events::WeaponHitEvent) if any active gunfire particle is touching them
  void check_gunfire_player_collision();

  //! @brief Hit (Events::WeaponHitEvent) any NPC the gun can hurt (damage_from_weapon.gun in npc.json) touched by an active gunfire particle
  void check_gunfire_npc_collision();

  //! @brief Levelgen 'path' cells (pixels), the only cells a Watchman spawns on.
  std::vector<sf::Vector2f> m_path_positions;

  //! @brief Ground world items (see Scene::m_item_sm).
  PathFinding::SpatialHashGridWeakPtr m_item_sm;

  //! @brief Sinkhole/corruption cells (see Scene::m_hazard_sm).
  PathFinding::SpatialHashGridWeakPtr m_hazard_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_WATCHMANSYSTEM_HPP__
