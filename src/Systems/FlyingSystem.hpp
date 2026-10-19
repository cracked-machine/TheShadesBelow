#ifndef SRC_SYSTEMS_FLYINGSYSTEM_HPP__
#define SRC_SYSTEMS_FLYINGSYSTEM_HPP__

#include <Events/PlayerActionEvent.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstdint>
#include <optional>

namespace Game::Sys
{

//! @brief Drives witchesbroom flight: holding Space with the broom equipped lifts the player off the ground.
//!
//! While flying (Cmp::Player::Flying on the player):
//! - the player sprite is lifted by Cmp::Persist::FlyingLiftOffset and oscillates vertically by up to
//!   Cmp::Persist::FlyingOscillationAmplitude. The lift is visual only - Cmp::Position stays on the ground.
//! - only walls, level boundaries and multiblocks larger than 1x1 block the player (see blocks_flight()).
//! - the player renders in front of all obstacles (kFlyingZOrder).
//! - the broom wears (its wear is "active_only" in items.json).
//!
//! When Space is released the player keeps flying until they are over a free grid cell, then descends onto it.
//! If the broom wears out over an obstacle the player crash lands (Cmp::Player::CrashLanding) on the nearest free cell.
//! A player who dies mid-flight doesn't land.
class FlyingSystem : public BaseSystem
{
public:
  //! @brief Z-order base for the player while flying. Above all obstacle/sprite z-orders (<= 10000) but below
  //! the post-process shaders (>= 20000) so mist/darkness are still applied over the player.
  static constexpr float kFlyingZOrder = 15000.f;
  //! @brief Radius (px) of the ground shadow drawn under the player while flying.
  static constexpr float kFlightShadowRadiusPx = 6.f;
  //! @brief Alpha of the ground shadow drawn under the player while flying.
  static constexpr std::uint8_t kFlightShadowAlpha = 110;

  //! @brief Construct a new Flying System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  FlyingSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Take off / land based on this frame's input and the broom's wear, and animate the sprite lift.
  //! @param dt
  void update( sf::Time dt );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief Whether `entity` still blocks the player while flying.
  //! Uses the sprite sheet's flying_collision_mask (per cell for multiblocks) when specified. Otherwise walls, level
  //! boundaries and segments of multiblocks larger than 1x1 block; everything else can be flown over.
  //! @param reg
  //! @param entity A Cmp::Player::NoPath entity.
  //! @return bool true if the entity blocks flight.
  static bool blocks_flight( entt::registry &reg, entt::entity entity );

  //! @brief Z-order for the elevated flying player: kFlyingZOrder + y, so they are drawn in front of obstacles,
  //! except walls and sprites whose spritesheets.json entry has an explicit zorder. Where the lifted sprite overlaps one of
  //! those and the player's ground ordering (y) is behind it, the player is kept just behind it.
  //! @param reg
  //! @return float
  static float elevated_zorder( entt::registry &reg );

  //! @brief World bounds of the ground shadow drawn under the flying player: centred on the bottom edge of the
  //! player's logical (ground) position, shifted by the resting render offset so it sits under the grounded sprite's feet.
  //! @param reg
  //! @return sf::FloatRect
  static sf::FloatRect shadow_bounds( entt::registry &reg );

  //! @brief Z-order for the flight shadow: just behind the player, but behind any obstacle cap it overlaps that
  //! has no obstacle main in its cell, so the cap hides the shadow on the ground behind it.
  //! @param reg
  //! @return float
  static float shadow_zorder( entt::registry &reg );

private:
  //! @brief Record that the fly key was held this frame.
  //! @param ev
  void on_player_action( Events::PlayerActionEvent ev );

  //! @brief Whether the inventory slot holds a witchesbroom that hasn't worn out.
  bool broom_usable();

  //! @brief Whether the player can take off: alive, broom usable and not committed to a wormhole jump.
  bool can_take_off();

  //! @brief Lift the player off the ground. The sprite ascends over Cmp::Persist::FlyingTransitionTime.
  void take_off();

  //! @brief Start descending to `target` and mark Cmp::Player::Flying as landing.
  //! @param target Top-left of the landing cell, or nullopt to descend in place.
  void begin_landing( std::optional<sf::Vector2f> target );

  //! @brief Lerp the ground position to the landing cell while the sprite sinks. Removes Cmp::Player::Flying on touchdown.
  //! @param dt
  void advance_descent( sf::Time dt );

  //! @brief The player's resting render offset from Cmp::Persist::PlayerRenderOffset; flight lift is applied on top of it.
  //! @return sf::Vector2f offset in pixels
  sf::Vector2f resting_render_offset();

  //! @brief Take-off/landing duration from Cmp::Persist::FlyingTransitionTime, guarded against zero.
  float transition_seconds();

  //! @brief Update the player's render offset: lift plus a smoothed random vertical oscillation.
  //! @param dt
  void update_render_offset( sf::Time dt );

  //! @brief Find the nearest grid cell the player can land on, searching outwards in rings. Hazard cells are
  //! skipped so a crash landing never relocates the player into a sinkhole.
  //! @return std::optional<sf::Vector2f> top-left of the cell, or nullopt if none was found.
  std::optional<sf::Vector2f> find_landing_cell();

  //! @brief Whether the player can land on `cell`: a world position free of blockers and NPCs.
  //! @param cell Top-left of the grid cell.
  //! @param allow_hazards true to accept hazard cells (e.g. sinkholes), false to treat them as blocked.
  bool is_free_landing_cell( const sf::Vector2f &cell, bool allow_hazards );

  //! @brief Where the player can land from their current position, if anywhere.
  //! Fully clear of obstacles: in place. Overlapping an obstacle edge by no more than Cmp::Persist::FlyingLandingOverlap
  //! (per side): the nearest grid cell, if free. Otherwise nullopt - keep flying.
  //! Hazard cells count as free: the player chose to land there, and falls in once landed.
  std::optional<sf::Vector2f> landing_spot_below();

  //! @brief Whether the player can land on `area` exactly: on the world, free of blockers and NPCs.
  //! @param area World-space rect, e.g. the player's current position.
  //! @param allow_hazards true to accept hazard cells (e.g. sinkholes), false to treat them as blocked.
  bool is_free_landing_area( const sf::FloatRect &area, bool allow_hazards );

  //! @brief Set by on_player_action() when the fly key is held, consumed by each update().
  bool m_fly_requested{ false };

  //! @brief Current height as a fraction of the full lift: 0 = on the ground, 1 = fully airborne.
  float m_lift_fraction{ 0.f };
  //! @brief Lift fraction when the descent started.
  float m_descent_start_fraction{ 0.f };
  //! @brief Descent progress: 0 = start, 1 = touchdown.
  float m_descent_progress{ 0.f };
  //! @brief Ground position when the descent started.
  sf::Vector2f m_land_start;
  //! @brief Ground position of the landing cell.
  sf::Vector2f m_land_target;

  //! @brief Current vertical displacement (px) of the oscillation.
  float m_displacement{ 0.f };
  //! @brief Displacement (px) the oscillation is currently easing towards.
  float m_displacement_target{ 0.f };
  //! @brief Time since a new displacement target was picked.
  sf::Time m_displacement_elapsed;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_FLYINGSYSTEM_HPP__
