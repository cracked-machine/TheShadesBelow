#ifndef SRC_UTILS_COLLISION_HPP__
#define SRC_UTILS_COLLISION_HPP__

#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <entt/entity/registry.hpp>

namespace Game::Utils::Collision
{

//! @brief Check if `pos` intersects the position of any entity that also owns a `Component`.
//! @tparam Component The component type an entity must also have (alongside Cmp::Position) to be considered.
//! @param reg reference to the entt reg
//! @param pos The bounds to test for intersection against each candidate's Cmp::Position
//! @param filter Optional callback to filter candidates. Return true to consider this entity, false to skip.
//! @return bool true if `pos` intersects the position of at least one matching, non-filtered-out entity
template <typename Component>
bool any_intersects( entt::registry &reg, Cmp::RectBounds pos, std::function<bool( const Component & )> filter = []( const Component & ) { return true; } )
{
  for ( auto [candidate_entt, candidate_cmp, candidate_pos] : reg.view<Component, Cmp::Position>().each() )
  {
    if ( not filter( candidate_cmp ) ) continue;
    if ( pos.findIntersection( candidate_pos ) ) { return true; }
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
bool pos_intersects( entt::registry &reg, Cmp::RectBounds pos, std::function<bool( const Component & )> filter = []( const Component & ) { return true; } )
{
  for ( auto [candidate_entt, candidate_cmp] : reg.view<Component>().each() )
  {
    if ( not filter( candidate_cmp ) ) continue;
    if ( pos.findIntersection( candidate_cmp ) ) { return true; }
  }
  return false;
}

} // namespace Game::Utils::Collision

#endif // SRC_UTILS_COLLISION_HPP__
