#include <Components/Npc/Npc.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <PathFinding/AStar.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Npc.hpp>
#include <Utils/Utils.hpp>

#include <algorithm>
#include <cmath>
#include <ranges>
#include <unordered_map>

namespace Game::PathFinding
{

using ClosedList = std::unordered_map<Cmp::Position, PathNode, PathNode::PosHash>;

std::vector<PathNode> astar( entt::registry &reg, const PathFinding::SpatialHashGrid &grid, Cmp::Position start, Cmp::Position goal,
                             PathFinding::QueryCompass query_compass, const std::function<bool( const Cmp::Position & )> &is_blocked,
                             std::span<const sf::Vector2i> footprint_cells )
{
  const bool multiblock = footprint_cells.size() > 1;
  const sf::Vector2f goal_cell_centre = goal.position + Constants::kGridSizePxF * 0.5f;

  // Path nodes are the footprint's anchor cell (index zero); its other cells keep their pos relative to that.
  // The position of footprint cell `cell` when the anchor cell stands on `node`:
  auto cell_at = [&]( const Cmp::Position &node, sf::Vector2i cell )
  {
    const sf::Vector2i rel_pos = cell - footprint_cells.front();
    return Cmp::Position( node.position + sf::Vector2f{ static_cast<float>( rel_pos.x ) * Constants::kGridSizePxF.x,
                                                        static_cast<float>( rel_pos.y ) * Constants::kGridSizePxF.y },
                          Constants::kGridSizePxF );
  };

  // A cell is walkable if the navmesh still has a non-NPC entry for it: blocked cells have their whole bucket removed.
  auto is_walkable = [&]( const Cmp::Position &cell )
  {
    if ( is_blocked && is_blocked( cell ) ) return false;
    return std::ranges::any_of( grid.at( cell ), [&]( entt::entity cell_entt )
    { return reg.valid( cell_entt ) and reg.all_of<Cmp::Position>( cell_entt ) and not reg.any_of<Cmp::Npc::NPC>( cell_entt ); } );
  };

  // A multiblock footprint anchored on `node` needs every other cell it covers to be walkable. The anchor cell
  // itself is the candidate neighbour, which the caller has already tested.
  auto has_clearance = [&]( const Cmp::Position &node )
  {
    return std::ranges::all_of( footprint_cells | std::views::drop( 1 ), [&]( sf::Vector2i cell ) { return is_walkable( cell_at( node, cell ) ); } );
  };

  // a multiblock footprint has arrived as soon as any cell of it is the goal cell
  auto footprint_at_goal = [&]( const Cmp::Position &node )
  {
    return std::ranges::any_of( footprint_cells | std::views::drop( 1 ),
                                [&]( sf::Vector2i cell ) { return cell_at( node, cell ).contains( goal_cell_centre ); } );
  };

  std::vector<PathNode> openList;
  ClosedList closedList;
  closedList.reserve( 512 ); // prevent rehashing which invalidates parent pointers

  std::size_t nodesExpanded = 0;

  PathNode startNode( start, 0, Utils::Maths::getManhattanDistance( start.position, goal.position ) );

  openList.push_back( startNode );

  PathNode *endNode = nullptr;
  static constexpr std::size_t kMaxNodes = 256; // bail out if goal is unreachable

  while ( not openList.empty() )
  {
    if ( ++nodesExpanded > kMaxNodes ) break; // goal unreachable, don't exhaust search space

    // Find PathNode with smallest f
    auto currentIt = std::ranges::min_element( openList, []( const PathNode &a, const PathNode &b ) { return a.f() < b.f(); } );
    PathNode current = *currentIt;
    openList.erase( currentIt );
    closedList.emplace( current.pos, current );

    const bool at_goal = ( current.x() == goal.x() && current.y() == goal.y() ) or ( multiblock and footprint_at_goal( current.pos ) );
    if ( at_goal )
    {
      endNode = &closedList.at( current.pos );
      break;
    }

    // nodes are top-left anchored and can be larger than one cell, so query from the anchor cell rather than the centre
    Cmp::Position center_hitbox( current.pos.position + Constants::kGridSizePxF * 0.5f, { 1.f, 1.f } );
    const std::vector<entt::entity> neighbours_list = grid.neighbours( center_hitbox, query_compass );

    for ( auto neighbour_entt : neighbours_list )
    {
      auto *neighbour_entity_pos = reg.try_get<Cmp::Position>( neighbour_entt );
      if ( not neighbour_entity_pos ) continue;

      // A node is the grid cell its entity is bucketed in, but not every navmesh entity is one aligned cell: the
      // player moves sub-grid, and a multiblock root spans several cells from its top-left. Reduce each to its
      // cell, otherwise the exact goal comparison never matches an off-grid player, and a multiblock root is
      // expanded from its centre, which sits inside its own colliding cells and lets the path jump through them.
      const Cmp::Position snapped_neighbour_pos( Utils::snap_to_grid( neighbour_entity_pos->position, Utils::Rounding::TOWARDS_ZERO ),
                                                 Constants::kGridSizePxF );
      const Cmp::Position *neighbour_pos = &snapped_neighbour_pos;

      // Skip other NPCs so they don't block each other's pathfinding
      if ( reg.any_of<Cmp::Npc::NPC>( neighbour_entt ) ) continue;

      if ( is_blocked && is_blocked( *neighbour_pos ) ) continue;
      if ( multiblock and not has_clearance( *neighbour_pos ) ) continue;

      auto heuristic = Utils::Maths::getManhattanDistance( neighbour_pos->position, goal.position );

      if ( closedList.contains( *neighbour_pos ) ) continue;

      // +1 since we only care about relative difference between steps, not the actual pixel distance.
      PathNode new_neighbor( *neighbour_pos, current.g + 1, heuristic, &closedList.at( current.pos ) );

      auto it = std::ranges::find_if( openList,
                                      // existence check
                                      [&]( const PathNode &n ) { return n == new_neighbor; } );

      // either add the new neighbour or add duplicate one if this is nearer to the goal
      if ( it == openList.end() || new_neighbor.g < it->g )
      {
        if ( it != openList.end() ) openList.erase( it );
        openList.push_back( new_neighbor );
      }
    }
  }

  // Reconstruct path
  std::vector<PathNode> path;
  PathNode *p = endNode;

  while ( p != nullptr )
  {
    path.push_back( *p );
    p = p->parent;
  }

  std::reverse( path.begin(), path.end() );
  return path;
}

} // namespace Game::PathFinding