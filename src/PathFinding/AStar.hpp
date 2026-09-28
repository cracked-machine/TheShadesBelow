#ifndef SRC_PATHFINDING_ASTAR_HPP__
#define SRC_PATHFINDING_ASTAR_HPP__

#include <Components/Position.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <entt/entity/entity.hpp>

#include <functional>

/// Main class
namespace Game::PathFinding
{

class SpatialHashGrid;

//! @brief A single node in the A* search: a grid position plus its cost bookkeeping and a link back to the
//! node it was reached from, so the final path can be reconstructed by walking `parent` pointers.
struct PathNode
{

  //! @brief World position of this node.
  Cmp::Position pos;

  //! @brief X coordinate of `pos`.
  //! @return The x pixel coordinate.
  float x() { return pos.position.x; }

  //! @brief Y coordinate of `pos`.
  //! @return The y pixel coordinate.
  float y() { return pos.position.y; }

  //! @brief Cost from start
  double g;
  //! @brief Heuristic cost to goal
  double h;
  //! @brief Pointer to parent node
  PathNode *parent{ nullptr };

  //! @brief Construct a new Path Node object
  //! @param pos World position of this node.
  //! @param g Cost from the start node.
  //! @param h Heuristic cost estimate to the goal.
  //! @param p Parent node this one was reached from, or nullptr for the start node.
  PathNode( Cmp::Position pos, double g = 0, double h = 0, PathNode *p = nullptr )
      : pos( pos ),
        g( g ),
        h( h ),
        parent( p )
  {
  }

  //! @brief get the combined g cost and heuristic estimate
  //! @return double
  double f() const { return g + h; }

  //! @brief Used for hash comparison in closedlist and std::find in openlist
  //! @param other
  //! @return true
  //! @return false
  bool operator==( const PathNode &other ) const
  {
    return static_cast<int>( pos.position.x ) == static_cast<int>( other.pos.position.x ) &&
           static_cast<int>( pos.position.y ) == static_cast<int>( other.pos.position.y );
  }

  //! @brief Used to create a hash for the ClosedList key. Use int to prevent rounding errors (also faster)
  struct PosHash
  {
    //! @brief
    //! @param p
    //! @return std::size_t
    std::size_t operator()( const Cmp::Position &p ) const noexcept
    {
      std::size_t h1 = std::hash<int>{}( static_cast<int>( p.position.x ) );
      std::size_t h2 = std::hash<int>{}( static_cast<int>( p.position.y ) );
      return h1 ^ ( h2 << 1 );
    }
  };
};

//! @brief Find a path from `start` to `goal` over the spatial hash grid using the A* algorithm, treating occupied
//! grid cells (excluding other NPCs) as obstacles. Bails out once a fixed node-expansion limit is reached if the
//! goal is unreachable.
//! @param reg The registry used to look up components (e.g. Cmp::Position, Cmp::Npc::NPC) on candidate neighbour entities.
//! @param grid The spatial hash grid used to query neighbouring occupied cells.
//! @param start Starting position.
//! @param goal Target position.
//! @param query_compass Which neighbour offsets to consider when expanding a node.
//! @param is_blocked Optional extra obstacle test; neighbour positions for which it returns true are never expanded.
//! @return The path from start to goal as an ordered list of PathNode, or an empty vector if no path was found.
std::vector<PathNode> astar( entt::registry &reg, const PathFinding::SpatialHashGrid &grid, Cmp::Position start, Cmp::Position goal,
                             PathFinding::QueryCompass query_compass = PathFinding::QueryCompass::CARDINAL,
                             const std::function<bool( const Cmp::Position & )> &is_blocked = {} );

} // namespace Game::PathFinding

#endif // SRC_PATHFINDING_ASTAR_HPP__
