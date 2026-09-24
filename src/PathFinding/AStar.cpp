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
#include <optional>
#include <unordered_map>

namespace Game::PathFinding
{

using ClosedList = std::unordered_map<Cmp::Position, PathNode, PathNode::PosHash>;

std::vector<PathNode> astar( entt::registry &reg, const PathFinding::SpatialHashGrid &spatial_grid, Cmp::Position start, Cmp::Position goal,
                             PathFinding::QueryCompass offset )
{

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

    if ( current.x() == goal.x() && current.y() == goal.y() )
    {
      endNode = &closedList.at( current.pos );
      break;
    }

    Cmp::Position center_hitbox( current.pos.getCenter(), { 1.f, 1.f } );
    const std::vector<entt::entity> neighbours_list = spatial_grid.neighbours( center_hitbox, offset );

    for ( auto neighbour_entt : neighbours_list )
    {
      auto *neighbour_entity_pos = reg.try_get<Cmp::Position>( neighbour_entt );
      if ( not neighbour_entity_pos ) continue;

      // The player moves sub-grid, so their entity sits at an off-grid position. When they stand on a cell that
      // has no other entity (e.g. where a moveable block used to be) their entity is the only node for that cell,
      // and the exact goal comparison below would never match. Snap it to its grid cell.
      std::optional<Cmp::Position> snapped_player_pos;
      const bool is_player = reg.any_of<Cmp::Player::Character>( neighbour_entt );
      if ( is_player )
      {
        snapped_player_pos.emplace( Utils::snap_to_grid( neighbour_entity_pos->position, Utils::Rounding::TOWARDS_ZERO ),
                                            neighbour_entity_pos->size );
      }
      const Cmp::Position *neighbour_pos = is_player ? &*snapped_player_pos : neighbour_entity_pos;

      // Skip other NPCs so they don't block each other's pathfinding
      if ( reg.any_of<Cmp::Npc::NPC>( neighbour_entt ) ) continue;

      auto heuristic = Utils::Maths::getManhattanDistance( neighbour_pos->position, goal.position );

      if ( closedList.count( *neighbour_pos ) > 0 ) continue;

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