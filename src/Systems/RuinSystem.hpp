#ifndef SRC_SYSTEMS_RUINSYSTEM_HPP__
#define SRC_SYSTEMS_RUINSYSTEM_HPP__

#include <Components/Ruin/FloorAccess.hpp>
#include <Sprites/SpriteMetaType.hpp>
#include <Systems/BaseSystem.hpp>

#include <future>

namespace Game::Sprites
{
class SpriteSheet;
class SpriteFactory;
} // namespace Game::Sprites

namespace Game::Cmp::Ruin
{
class StairsLowerMultiBlock;
class StairsUpperMultiBlock;
class StairsBalustradeMultiBlock;
} // namespace Game::Cmp::Ruin

namespace Pathfinding
{
class SpatialHashGrid;
} // namespace Pathfinding

namespace Game::Sys
{

//! @brief Handles the ruin's upper/lower floor transitions, bookcase decoration, rune-marking puzzle,
//! player curse and the shadow hand hazard.
class RuinSystem : public BaseSystem
{
public:
  //! @brief Construct a new Ruin System object
  //! @param reg
  //! @param window
  //! @param sprite_factory
  //! @param sound_bank
  //! @param scenemanager_event_dispatcher
  RuinSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank,
              entt::dispatcher &scenemanager_event_dispatcher );

  //! @brief init the weak pointer for the spatial grid
  //! @param pathfinding_navmesh
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &pathfinding_navmesh, const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr )
  {
    m_pathfinding_navmesh = pathfinding_navmesh;
    m_reserved_sm = reserved_sm;
  }

  //! @brief Check for player collision with the ruin entrance
  void check_entrance_collision();

  //! @brief Sync the exit multiblock's z-order to the segment the player is currently standing on.
  void update_exit_zorder();

  //! @brief Create a floor access (staircase) entity at the given position.
  //! @param spawn_position
  //! @param size
  //! @param dir
  void spawn_floor_access( sf::Vector2f spawn_position, sf::Vector2f size, Cmp::Ruin::FloorAccess::Direction dir );

  //! @brief Pan the creaking rope sound effect back and forth over time.
  void creaking_rope_update();

  //! @brief Trigger a floor transition scene event when the player steps onto a floor access tile leading
  //! in `direction`, debounced by m_floor_access_cooldown.
  //! @param direction
  void check_floor_access_collision( Cmp::Ruin::FloorAccess::Direction direction );

  //! @brief Reset the floor access cooldown (call when entering a ruin scene)
  void reset_floor_access_cooldown()
  {
    m_floor_access_cooldown.restart();
    m_was_on_floor_access = true; // Assume we just landed on a floor access
  }

  //! @brief Procedurally place rows of bookcase obstacles across the lower floor scene.
  //! @param scene_dimensions
  void gen_lowerfloor_bookcases( sf::FloatRect scene_dimensions );

  //! @brief Destroy cobweb obstacles surrounded on more than three sides by rune markings, so puzzle runes stay reachable.
  //! @param non_obstacle_spatialgrid
  void remove_rune_markings_neighbouring_cobwebs( PathFinding::SpatialHashGrid &non_obstacle_spatialgrid );

  //! @brief Apply a player movement speed penalty while standing on ruin staircases.
  void check_movement_slowdowns();

  //! @brief Clear the player's shadow curse and reset the curse-activation future.
  void reset_player_curse();

  //! @brief Check for player collision with the ruin exit
  void check_exit_collision();

  //! @brief Enable/disable the stairs gate depending on whether all rune markings are active, playing a
  //! secret sound the first time it unlocks.
  void check_puzzle_status();

  //! @brief Create a witch entt if none exist
  //! @param reg s
  //! @param scene_boundary
  void check_create_witch( sf::FloatRect scene_boundary );

  //! @brief Create spider NPCs using Cmp::Persist::RuinMaxSpiders as a limit.
  //! @param reg
  //! @param scene_boundary
  void create_spiders( sf::FloatRect scene_boundary );

  //! @brief One time, one shot function that asynchronously enables player curse and then sleeps for N seconds.
  //!        This allows for cinematic pause after curse activation.
  //! @details First frame: the future is invalid, so the async task is launched and the future becomes
  //! valid. Subsequent frames while still sleeping: the future is valid, so launching is skipped and
  //! `wait_for` returns `future_status::deferred` or `future_status::timeout`. Once the sleep has
  //! elapsed: the future is valid and `wait_for` returns `future_status::ready`.
  //! @param scene_dimensions
  //! @return True if the player curse is currently active, false otherwise.
  bool check_activate_player_curse( sf::Vector2f scene_dimensions );

  //! @brief Advance the shadow hand NPC's position towards the edge of the scene.
  //! @param scene_dimensions
  void update_shadow_hand_pos( sf::Vector2f scene_dimensions );

  //! @brief Apply the shadow hand's collision action to the player and trigger death if health reaches zero.
  //! @param dt
  void check_player_shadow_hand_collision( sf::Time dt );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Dispatcher reference for scene management events
  entt::dispatcher &m_scenemanager_event_dispatcher;

  //! @brief Cooldown clock to prevent immediate floor access re-triggering
  sf::Clock m_floor_access_cooldown{};

  //! @brief Minimum time, in seconds, between floor access re-triggers.
  static constexpr float kFloorAccessCooldownSeconds = 0.5f;

  //! @brief Track if player was on floor access last frame (requires leaving before re-triggering)
  bool m_was_on_floor_access{ false };

  //! @brief Elapsed-time source for the creaking rope's stereo pan oscillation.
  sf::Clock m_creaking_rope_swing_timer{};

  //! @brief Handle to the async task that delays curse effects (screams, shadow hand spawn) after
  //! Cmp::Player::Curse is activated.
  std::future<void> m_curse_activation_future;

  //! @brief Weak pointer to the pathfinding navmesh.
  PathFinding::SpatialHashGridWeakPtr m_pathfinding_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;

  //! @brief Accumulated time since the shadow hand's collision action effects were last applied.
  sf::Time m_shadowhand_action_effects_time;

  //! @brief True once all rune markings are active and the stairs gate has been unlocked.
  bool m_puzzle_solved{ false };
};
} // namespace Game::Sys

#endif // SRC_SYSTEMS_RUINSYSTEM_HPP__
