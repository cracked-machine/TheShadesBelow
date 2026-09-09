#ifndef SRC_SYSTEMS_THREATS_WATCHMANSYSTEM_HPP__
#define SRC_SYSTEMS_THREATS_WATCHMANSYSTEM_HPP__

#include <Components/Position.hpp>
#include <Components/UUID.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/System/Time.hpp>

namespace Game::Sys
{

//! @brief Drives the Watchman NPC: rate-limited spawning, searchlight sweep/lock-on behaviour,
//! gunfire cocking/firing timing, and gunfire collision against the player and Skeleton NPCs.
class WatchmanSystem : public BaseSystem
{
public:
  //! @brief Construct a new Watchman System object
  //! @param reg
  //! @param window
  //! @param sprite_factory
  //! @param sound_bank
  WatchmanSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointer for the reserved-positions grid.
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &reserved_sm ) { m_reserved_sm = reserved_sm; }

  //! @brief Rate-limit and trigger Watchman NPC spawning in the graveyard scene
  //! @param dt Delta time since last update call
  void update( sf::Time dt );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Rate-limit the spawning of Watchman NPCs
  sf::Time m_watchman_spawn_timer;

  //! @brief Create a watchman NPC at a random location.
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

  //! @brief Damage the player if any active gunfire particle is touching them
  void check_gunfire_player_collision();

  //! @brief Kill any Skeleton NPC touched by an active gunfire particle
  void check_gunfire_npc_collision();

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_WATCHMANSYSTEM_HPP__
