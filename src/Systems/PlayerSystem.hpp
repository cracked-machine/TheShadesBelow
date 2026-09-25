#ifndef SRC_SYSTEMS_PLAYERSYSTEM_HPP__
#define SRC_SYSTEMS_PLAYERSYSTEM_HPP__

#include <Components/Direction.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <entt/entity/fwd.hpp>
#include <optional>

// clang-format off
namespace Game::Events { class DropInventoryEvent; class PlayerActionEvent; class PlayerMortalityEvent; }
namespace Game::Sprites { class SpriteSheet; }
namespace Game::Cmp { class LerpPosition; class Position; class AnimData; class RectBounds; class BaseAction; }
namespace Game::Cmp::Npc { class Shockwave; }
namespace Game::Cmp::Player { class Mortality; }
namespace Game::Cmp::Peristent { class EffectsVolume; } 
namespace Game::PathFinding { class SpatialHashGrid; }
// clang-format on

namespace Game::Sys
{

//! @brief Drives player movement, animation, obstacle push/pull, hazard collision and mortality.
class PlayerSystem : public BaseSystem
{
public:
  //! @brief Construct a new Player System object
  //! @param reg
  //! @param window
  //! @param sprite_factory
  //! @param sound_bank
  //! @param scenemanager_event_dispatcher
  PlayerSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank,
                entt::dispatcher &scenemanager_event_dispatcher );

  //! @brief init the weak pointer for the pathfinding navmesh
  //! @param npc_navmesh
  //! @param player_navmesh
  //! @param open_navmesh
  void init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh, const PathFinding::SpatialHashGridSharedPtr &player_navmesh,
             const PathFinding::SpatialHashGridSharedPtr &open_navmesh )
  {
    m_npc_navmesh = npc_navmesh;
    m_player_navmesh = player_navmesh;
    m_open_navmesh = open_navmesh;
  }

  //! @brief Update the player system.
  //! @param dt
  void update( sf::Time dt );

  //! @brief Stop the player's damage cooldown timer.
  void disable_damage_cooldown();

  //! @brief Restart the player's damage cooldown timer.
  void enable_damage_cooldown();

  //! @brief Force the player's very next damage check to land regardless of the damage cooldown timer.
  void force_expire_damage_cooldown();

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief True while a recent push/pull obstacle move is still suppressing player movement.
  bool movement_suppressed();

  //! @brief Computes the scaled movement vector for the current frame from the player's raw input
  //! direction. Returns std::nullopt if the player has no directional input.
  //! @param dt frame delta time
  //! @param apply_speed_penalty whether to factor in Utils::Player::get_speed_penalty - only the main
  //!        position update does; push/pull obstacle checks move at the base movement speed
  std::optional<Cmp::Direction> compute_step_direction( sf::Time dt, bool apply_speed_penalty );

  //! @brief Compute and apply the player's per-frame movement, resolving collisions against
  //! obstacles/hazards per axis and sliding along a wall via an edge-nudge when blocked on one axis.
  //! @param dt
  void update_player_position( sf::Time dt );

  //! @brief Change to animation spritesheet for the players current direction.
  void update_player_animation();

  //! @brief Check if the player is dead
  //! @note This checks if the Cmp::Player::Mortality == State::DEAD, not check Cmp:PlayerStats (that is other system responsibility).
  void check_player_mortality();

  //! @brief Update InventoryItem/NPC action effects on player for Tick::SLOW.
  //! @note Cmp::CollisionAction and Cmp::ProjectileAction are excluded. See NpcSystem.
  //! @param dt
  void check_timed_action_side_effects( sf::Time dt );

  //! @brief Decide when to apply the fear stat (in darkness) or not (candle light, etc...)
  //! @param net_modifier The net modifications from each light boundary
  //! @param candle_carry_action Properties used to apply the modifications.
  //! @return std::stringstream Use .rdbuf() at the callsite (prvalue expression semantics)
  std::stringstream apply_fear_of_the_dark( Cmp::BaseAction &net_modifier, const Cmp::BaseAction &candle_carry_action );

  bool is_player_in_light();

  //! @brief Apply the stat modifiers for the healing spring
  //! @param net_modifier The accumulated net modifications
  void apply_healing_spring_modifiers( Cmp::BaseAction &net_modifier );

  //! @brief Apply the stat modifiers for visible NPCs
  //! @param net_modifier The accumulated net modifications
  //! @return std::stringstream Use .rdbuf() at the callsite (prvalue expression semantics)
  std::stringstream apply_npc_modifiers( Cmp::BaseAction &net_modifier );

  //! @brief Apply the stat modifiers for inventory item
  //! @param net_modifier The accumulated net modifications
  //! @return std::stringstream Use .rdbuf() at the callsite (prvalue expression semantics)
  std::stringstream apply_inventory_modifiers( Cmp::BaseAction &net_modifier );

  void set_player_on_fire();

  //! @brief Update the clocks for the timed actions
  //! @param dt
  void update_timed_action_clocks( sf::Time dt );

  //! @brief Send mortality event if player fear/despair is 100%
  void kill_player_if_max_fear_despair();

  //! @brief Add/remove the player healing particle sprite depending on proximity to an active healing spring fountain.
  void create_healing_particles();

  //! @brief Move a selected Cmp::Moveable obstacle by one step in the direction the player is pushing/pulling.
  //! @param target_position
  void move_obstacle( const sf::FloatRect &target_position );

  //! @brief Check if the player is pushing a moveable obstacle and move it if so.
  //! @param dt
  void check_player_can_push( sf::Time dt );

  //! @brief Check if the player is pulling a moveable obstacle and move it if so.
  //! @param dt
  void check_player_can_pull( sf::Time dt );

  //! @brief Fade the player alpha if they intiiated wormhole jump.
  void fade_player_on_wormhole_jump();

  //! @brief Blink the player if damage cooldown was activated.
  void blink_player();

  //! @brief Checks if the player's movement to a given position is valid
  //! Validates whether the player can move to the specified position by checking
  //! for collisions with walls, boundaries, or other obstacles in the game world.
  //! @param player_position The target position to validate for player movement
  //! @return true if the movement is valid and allowed, false otherwise
  bool is_valid_move( const sf::FloatRect &target_position );

  //! @brief Resolves the initial pushback/subsequent yield against `Cmp::Hazard::CollisionResist`
  //! entities (lethal hazard cells). The first contact with a given hazard cell blocks movement like
  //! a wall; once the player has kept pushing into that same cell for its configured resist duration,
  //! this yields and allows the move through so it counts as normal (fatal) hazard collision.
  //! @param search_bounds The (already scaled) target move bounds to test against hazard cells
  //! @return true if movement should be allowed, false if still being resisted
  bool resolve_hazard_pushback( const Cmp::RectBounds &search_bounds );

  //! @brief Various ways in which the player can die. Starts the death animation loop.
  //! @param ev
  void on_player_mortality_event( Game::Events::PlayerMortalityEvent ev );

  //! @brief Promotes any Cmp::Player::PendingNoPath entity to a real Cmp::Player::NoPath - if the player is no longer
  //! standing on it - and inserts it into m_player_navmesh.
  void promote_pending_no_path();

  //! @brief Use this to send events to the scene manager
  entt::dispatcher &m_scenemanager_event_dispatcher;

  //! @brief Synchronizes the action stat modifiers.
  sf::Time m_timed_action_sync_clock;

  //! @brief Global fear stat increase timer when player is standing in darkness.
  sf::Time m_darkness_fear_clock;

  //! @brief Weak pointer to the pathfinding navmesh.
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief All grid positions that block player movement
  PathFinding::SpatialHashGridWeakPtr m_player_navmesh;

  //! @brief All grid positions that are open (not blocked) for movement
  PathFinding::SpatialHashGridWeakPtr m_open_navmesh;

  //! @brief Hazard cell currently being pushed against by `resolve_hazard_pushback`, or entt::null.
  entt::entity m_hazard_pushback_target;

  //! @brief How long the player has continuously pushed into m_hazard_pushback_target.
  sf::Clock m_hazard_pushback_clock;

  //! @brief Tracks how long the bow has been held drawn, between DRAW_BOW and RELEASE_BOW.
  sf::Clock m_bow_draw_clock;

  //! @brief True while the bow is being drawn (left mouse button held with bow equipped).
  bool m_bow_drawing{ false };

  //! @brief Axis of the last successfully committed player movement, used only to decide which axis
  //! `update_player_position` resolves first each frame (see the axis-favoring fix in that function).
  //! Deliberately separate from Cmp::LastDirection, which tracks facing intent for dig/interact checks
  //! and must keep updating even when the player is blocked - unlike this, which must not.
  Cmp::Direction m_last_committed_axis{ sf::Vector2f{ 1.f, 0.f } };

  const std::string kPlayerFireTag = "graveyard.player.particle.flame";
  bool m_player_is_on_fire = false;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_PLAYERSYSTEM_HPP__
