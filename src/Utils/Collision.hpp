#ifndef SRC_UTILS_COLLISION_HPP__
#define SRC_UTILS_COLLISION_HPP__

#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <entt/entity/registry.hpp>

#include <span>
#include <type_traits>
#include <vector>

namespace Game::Utils::Collision
{

//! @brief Check if `pos` intersects the position of any entity that also owns a `Component`.
//! @tparam Component The component type an entity must also have (alongside Cmp::Position) to be considered.
//! @param reg reference to the entt reg
//! @param pos The bounds to test for intersection against each candidate's Cmp::Position
//! @param filter Optional callback to filter candidates. Return true to consider this entity, false to skip.
//! @return bool true if `pos` intersects the position of at least one matching, non-filtered-out entity
template <typename Component>
bool any_intersects( entt::registry &reg, Cmp::RectBounds pos,
                     std::function<bool( const Component & )> filter = []( const Component & ) { return true; } )
{
  if constexpr ( std::is_empty_v<Component> )
  {
    // empty tag components (e.g. Cmp::Player::NoPath) aren't yielded by each(), so pass a default instance to the filter
    for ( auto [candidate_entt, candidate_pos] : reg.view<Component, Cmp::Position>().each() )
    {
      if ( not filter( Component{} ) ) continue;
      if ( pos.findIntersection( candidate_pos ) ) { return true; }
    }
  }
  else
  {
    for ( auto [candidate_entt, candidate_cmp, candidate_pos] : reg.view<Component, Cmp::Position>().each() )
    {
      if ( not filter( candidate_cmp ) ) continue;
      if ( pos.findIntersection( candidate_pos ) ) { return true; }
    }
  }
  return false;
};

//! @brief Invoke `fn` for every entity that owns a `Component` (alongside Cmp::Position) whose position intersects `pos`.
//! @tparam Component The component type an entity must also have (alongside Cmp::Position) to be considered.
//! @param reg reference to the entt reg
//! @param pos The bounds to test for intersection against each candidate's Cmp::Position
//! @param fn Callback invoked as `fn(entt::entity, Component&, Cmp::Position&)` for each intersecting match.
template <typename Component, typename Fn>
void for_each_intersect( entt::registry &reg, const sf::FloatRect &pos, Fn &&fn )
{
  for ( auto [candidate_entt, candidate_cmp, candidate_pos] : reg.view<Component, Cmp::Position>().each() )
  {
    if ( pos.findIntersection( candidate_pos ) ) { fn( candidate_entt, candidate_cmp, candidate_pos ); }
  }
}

//! @brief Concept satisfied if `T` inherits from one of the valid position/bounds types.
//! @tparam T The type to test.
template <typename T>
concept HasPositionBounds = std::is_base_of_v<Cmp::Position, T> || std::is_base_of_v<Cmp::RectBounds, T> || std::is_base_of_v<sf::FloatRect, T>;

//! @brief Check if `pos` intersects any entity's `Component`, where `Component` itself is a position/bounds type.
//! @tparam Component Must satisfy HasPositionBounds (inherit from Cmp::Position, Cmp::RectBounds, or sf::FloatRect)
//! @param reg reference to the entt reg
//! @param pos The bounds to test for intersection against each candidate's `Component`
//! @param filter Optional callback to filter candidates. Return true to consider this entity, false to skip.
//! @return bool true if `pos` intersects at least one matching, non-filtered-out entity's `Component`
template <typename Component>
  requires HasPositionBounds<Component>
bool pos_intersects( entt::registry &reg, Cmp::RectBounds pos,
                     std::function<bool( const Component & )> filter = []( const Component & ) { return true; } )
{
  for ( auto [candidate_entt, candidate_cmp] : reg.view<Component>().each() )
  {
    if ( not filter( candidate_cmp ) ) continue;
    if ( pos.findIntersection( candidate_cmp ) ) { return true; }
  }
  return false;
}

//! @brief Hitbox used to test the player against lethal hazard cells: `rect` inset on every side by
//! Cmp::Persist::HazardBalanceOverlap (but never smaller than 1px), i.e. the player only counts as over a hazard once
//! they have stepped further onto it than that. At 0 it is `rect` itself; at the 0.5 maximum it is a 1px square at
//! the centre of `rect`. Shared by the movement blocking/balance window (Sys::PlayerSystem) and the hazard death checks
//! so they always agree.
//! @param reg reference to the entt reg
//! @param rect The player's (current or target) bounds.
//! @return Cmp::RectBounds The inset probe.
Cmp::RectBounds lethal_hazard_probe( entt::registry &reg, const sf::FloatRect &rect );

//! @brief Find a lethal hazard cell (Cmp::SinkholeCell or Cmp::Crypt::RoomLavaPitCell) intersecting `rect`.
//! Corruption (Cmp::CorruptionCell) is a gradual damage field, so it is not a lethal hazard.
//! @param reg reference to the entt reg
//! @param rect The bounds to test for intersection.
//! @return entt::entity The first intersecting lethal hazard cell, or entt::null if none.
entt::entity find_lethal_hazard( entt::registry &reg, const Cmp::RectBounds &rect );

//! @brief Check whether `rect` intersects any lethal hazard cell. See find_lethal_hazard().
//! @param reg reference to the entt reg
//! @param rect The bounds to test for intersection.
//! @return bool true if `rect` intersects a sinkhole or lava pit cell.
inline bool any_lethal_hazard( entt::registry &reg, const Cmp::RectBounds &rect ) { return find_lethal_hazard( reg, rect ) != entt::null; }

//! @brief Snapshot of every active light source, gathered once so that repeated illumination
//! queries (e.g. per A* neighbour) don't re-walk the registry.
struct LightSources
{
  //! @brief Centres of circular light sources; all share the player's torch radius.
  std::vector<sf::Vector2f> circles;
  //! @brief Radius applied to every entry in `circles`.
  float radius{ 0.f };
  //! @brief Lava pit cells; these light anything whose 1.5x grid hitbox overlaps them.
  std::vector<Cmp::Position> lava;

  //! @brief Check whether `pos` is lit by any light source.
  //! @param pos The position to test.
  //! @return bool true if any source lights `pos`.
  bool illuminates( const Cmp::Position &pos ) const;

  //! @brief Check whether `pos` is lit by a light source that lights none of the `exempt` positions.
  //! Used by NPC pathfinding: lights covering the target or the NPC itself are passable, all others are walls.
  //! @param pos The position to test.
  //! @param exempt Positions whose own light sources should be ignored.
  //! @return bool true if `pos` should be treated as blocked.
  bool blocks( const Cmp::Position &pos, std::span<const Cmp::Position> exempt ) const;

private:
  bool circle_lights( const sf::Vector2f &centre, const Cmp::Position &pos ) const;
  static bool lava_lights( const Cmp::Position &lava_cell, const Cmp::Position &pos );
};

//! @brief Gather all currently visible light sources
//! (inventory torch, visible torch, altar flame, lava pit, burning plant, or wisp).
//! @param reg reference to the entt registry
//! @return LightSources the snapshot
LightSources collect_light_sources( entt::registry &reg );

//! @brief Check whether the given position is within torch radius of any active light source
//! (visible torch, altar flame, lava pit, burning plant, or wisp), including sources just off-screen whose light reaches on-screen.
//! @param reg reference to the entt registry
//! @param pos_cmp The position to test.
//! @return bool true if `pos_cmp` is within range of any light source.
bool is_position_illuminated( entt::registry &reg, const Cmp::Position &pos_cmp );

} // namespace Game::Utils::Collision

#endif // SRC_UTILS_COLLISION_HPP__
