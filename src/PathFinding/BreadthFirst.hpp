#ifndef SRC_PATHFINDING_BREADTHFIRST_HPP__
#define SRC_PATHFINDING_BREADTHFIRST_HPP__

#include <SFML/System/Vector2.hpp>
#include <entt/entity/fwd.hpp>

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

namespace Game::PathFinding
{

class SpatialHashGrid;

//! @brief A cell reached by breadth_first_field(): where it was reached from and how far it is from the start.
struct FieldNode
{
  //! @brief Grid coords of the cell this one was reached from. The start cell is its own parent.
  sf::Vector2i parent;
  //! @brief Number of cardinal steps from the start cell.
  int steps{ 0 };
};

//! @brief Hash for grid coords, so they can key a DistanceField.
struct CellHash
{
  //! @brief
  //! @param cell
  //! @return std::size_t
  std::size_t operator()( sf::Vector2i cell ) const noexcept
  {
    std::size_t h1 = std::hash<int>{}( cell.x );
    std::size_t h2 = std::hash<int>{}( cell.y );
    return h1 ^ ( h2 << 1 );
  }
};

//! @brief Every cell reachable from the start of a breadth_first_field(), keyed by grid coords.
using DistanceField = std::unordered_map<sf::Vector2i, FieldNode, CellHash>;

//! @brief Breadth-first search over the walkable cells of `grid`, outwards from the cell containing `start_px`.
//! Unlike astar() this visits everything reachable, so one search gives the shortest route to any number of goals
//! (see route_to()). A cell is walkable under the same rule as astar(): its bucket holds a non-NPC entity with a
//! Cmp::Position. The start cell is always included. Steps are cardinal only.
//! @param reg The registry used to look up components on the entities bucketed in `grid`.
//! @param grid The navmesh to search.
//! @param start_px World position inside the start cell.
//! @return The reached cells. Never empty: it holds at least the start cell.
DistanceField breadth_first_field( entt::registry &reg, const SpatialHashGrid &grid, sf::Vector2f start_px );

//! @brief Shortest route through a DistanceField, from its start cell to the cell containing `goal_px`.
//! If the goal cell was not reached (e.g. it is a solid object standing on the edge of the walkable area), the
//! route goes to its nearest reached cardinal neighbour and then steps onto the goal cell.
//! @param field The result of breadth_first_field().
//! @param goal_px World position inside the goal cell.
//! @return Cell centres in world coords from the start cell to the goal cell, keeping only the start, the corners
//! and the goal. Empty if neither the goal cell nor any cardinal neighbour of it was reached.
std::vector<sf::Vector2f> route_to( const DistanceField &field, sf::Vector2f goal_px );

} // namespace Game::PathFinding

#endif // SRC_PATHFINDING_BREADTHFIRST_HPP__
