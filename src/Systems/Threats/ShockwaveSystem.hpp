#ifndef SRC_SYSTEMS_THREATS_SHOCKWAVESYSTEM_HPP__
#define SRC_SYSTEMS_THREATS_SHOCKWAVESYSTEM_HPP__

#include <Components/Npc/Shockwave.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <Sprites/Shockwave.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sys
{

//! @brief Grows and animates the priest NPC's radial shockwave attack, splitting its ring into
//! segments where it hits obstacles and checking those segments against the player for collision/knockback.
class ShockwaveSystem : public BaseSystem
{
public:
  //! @brief Construct a new Shockwave System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  ShockwaveSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Update Priest NPC shockwave
  //! @param dt
  void update( sf::Time dt );

  //! @brief Remove segments that intersect with a rectangle
  //! @param rect
  //! @param shockwave
  static void remove_intersecting_segments( const sf::FloatRect &rect, Cmp::Npc::Shockwave &shockwave );

  //! @brief Entrypoint for collision checking
  void check_shockwave_player_collision();

  //! @brief event handlers for pausing system clocks
  void on_pause() override {};
  //! @brief event handlers for resuming system clocks
  void on_resume() override {};

private:
  //! @brief Tracks time since the last shockwave radius/collision update.
  sf::Clock shockwave_update_clock;

  //! @brief Check if shockwave has collided with an obstacle and needs to be segmented.
  //! @param shockwave_entity
  //! @param shockwave
  void check_shockwave_obstacle_collision( entt::entity shockwave_entity, Cmp::Npc::Shockwave &shockwave );

  //! @brief Check if the player position intersects with the shockwave segments. Knockback player if there is a collision.
  //! @param reg
  //! @param shockwave
  //! @param player_pos
  //! @return true If any segment intersected with player position
  //! @return false If no segments intersected with player position
  bool intersects_with_visible_segments( const Cmp::Npc::Shockwave &shockwave, const sf::FloatRect &player_pos );

  //! @brief Checks segments for intersections with a rectangle at sample intervals. Splits existing segments where samples intersect.
  //! @param segment
  //! @param obstacle_rect
  //! @param shockwave_position
  //! @param radius
  //! @param samples
  //! @return Sprites::Shockwave::CircleSegments New list of non-intersecting segments
  static Sprites::Shockwave::CircleSegments split_segment_by_obstacle( const Sprites::CircleSegment &segment, const sf::FloatRect &obstacle_rect,
                                                                       sf::Vector2f shockwave_position, float radius, int samples );
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_SHOCKWAVESYSTEM_HPP__
