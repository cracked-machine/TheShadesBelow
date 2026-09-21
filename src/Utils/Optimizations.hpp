#ifndef SRC_UTILS_OPTIMIZATIONS_HPP__
#define SRC_UTILS_OPTIMIZATIONS_HPP__

#include <Utils/Profiling.hpp>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/View.hpp>

namespace Game::Utils
{

//! @brief Calculates the bounding rectangle of the given SFML view.
//! @details This function computes the world-space bounds represented by the specified sf::View.
//! The returned sf::FloatRect describes the area visible through the view.
//! @param view Reference to the SFML view whose bounds are to be calculated.
//! @return sf::FloatRect The rectangle representing the view's bounds in world coordinates.
constexpr inline sf::FloatRect calculate_view_bounds( const sf::View &view )
{
  //
  return { view.getCenter() - view.getSize() / 2.f, view.getSize() };
}

//! @brief Determines if a given position rectangle is visible within the specified view bounds.
//! @details This function checks whether the provided position rectangle intersects with the view bounds,
//! indicating that the position is at least partially visible in the current view.
//! @param viewbounds The rectangle representing the bounds of the current view.
//! @param position The rectangle representing the position to check for visibility.
//! @return true if the position is visible within the view bounds; false otherwise.
constexpr inline bool is_visible_in_view( const sf::FloatRect &viewbounds, const sf::FloatRect &position )
{
  ZoneScopedN( "Utils::is_visible_in_view( const sf::FloatRect &viewbounds, const sf::FloatRect &position )" );
  return viewbounds.findIntersection( position ).has_value();
}

//! @brief Checks if a position is visible within a given view's bounds
//! @param view The view to check against (in world coordinates)
//! @param position The position to test for visibility
//! @return true if the position's hitbox intersects with the view bounds
constexpr inline bool is_visible_in_view( const sf::View &view, const sf::FloatRect &position )
{
  ZoneScopedN( "Utils::is_visible_in_view( const sf::View &view, const sf::FloatRect &position )" );
  auto viewBounds = calculate_view_bounds( view );
  return viewBounds.findIntersection( position ).has_value();
}

} // namespace Game::Utils

#endif // SRC_UTILS_OPTIMIZATIONS_HPP__
