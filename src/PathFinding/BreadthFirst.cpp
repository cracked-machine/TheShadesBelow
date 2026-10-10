#include <Components/Npc/Npc.hpp>
#include <Components/Position.hpp>
#include <PathFinding/BreadthFirst.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Utils/Cardinal.hpp>
#include <Utils/Constants.hpp>

#include <entt/entity/registry.hpp>

#include <algorithm>
#include <cmath>
#include <deque>
#include <optional>

namespace Game::PathFinding
{

namespace
{

sf::Vector2i cell_of( sf::Vector2f px )
{
  return { static_cast<int>( std::floor( px.x / Constants::kGridSizePxF.x ) ), static_cast<int>( std::floor( px.y / Constants::kGridSizePxF.y ) ) };
}

sf::Vector2f top_left_of( sf::Vector2i cell )
{
  return { static_cast<float>( cell.x ) * Constants::kGridSizePxF.x, static_cast<float>( cell.y ) * Constants::kGridSizePxF.y };
}

sf::Vector2f centre_of( sf::Vector2i cell ) { return top_left_of( cell ) + Constants::kGridSizePxF * 0.5f; }

} // namespace

DistanceField breadth_first_field( entt::registry &reg, const SpatialHashGrid &grid, sf::Vector2f start_px )
{
  // A cell is walkable if the navmesh still has a non-NPC entry for it: blocked cells have their whole bucket removed.
  auto is_walkable = [&]( sf::Vector2i cell )
  {
    return std::ranges::any_of( grid.at( Cmp::Position( top_left_of( cell ), Constants::kGridSizePxF ) ), [&]( entt::entity cell_entt )
    { return reg.valid( cell_entt ) and reg.all_of<Cmp::Position>( cell_entt ) and not reg.any_of<Cmp::Npc::NPC>( cell_entt ); } );
  };

  const sf::Vector2i start = cell_of( start_px );

  DistanceField field;
  field.emplace( start, FieldNode{ .parent = start, .steps = 0 } );

  std::deque<sf::Vector2i> frontier{ start };
  while ( not frontier.empty() )
  {
    const sf::Vector2i current = frontier.front();
    frontier.pop_front();
    const int next_steps = field.at( current ).steps + 1;

    for ( auto c : Utils::Cardinal() )
    {
      const sf::Vector2i neighbour = current + sf::Vector2i( c.vector() );
      if ( field.contains( neighbour ) ) continue;
      if ( not is_walkable( neighbour ) ) continue;
      field.emplace( neighbour, FieldNode{ .parent = current, .steps = next_steps } );
      frontier.push_back( neighbour );
    }
  }
  return field;
}

std::vector<sf::Vector2f> route_to( const DistanceField &field, sf::Vector2f goal_px )
{
  const sf::Vector2i goal = cell_of( goal_px );

  // the last walkable cell of the route: the goal itself, or failing that its nearest reached neighbour
  std::optional<sf::Vector2i> last;
  if ( field.contains( goal ) ) { last = goal; }
  else
  {
    for ( auto c : Utils::Cardinal() )
    {
      auto it = field.find( goal + sf::Vector2i( c.vector() ) );
      if ( it == field.end() ) continue;
      if ( not last or it->second.steps < field.at( *last ).steps ) last = it->first;
    }
  }
  if ( not last ) return {};

  // walk back to the start, which is its own parent
  std::vector<sf::Vector2i> cells;
  if ( *last != goal ) cells.push_back( goal );
  for ( sf::Vector2i cell = *last;; cell = field.at( cell ).parent )
  {
    cells.push_back( cell );
    if ( field.at( cell ).parent == cell ) break;
  }
  std::ranges::reverse( cells );

  // keep only the cells where the route starts, turns or ends
  std::vector<sf::Vector2f> route;
  for ( std::size_t i = 0; i < cells.size(); ++i )
  {
    const bool is_end = ( i == 0 ) or ( i + 1 == cells.size() );
    if ( is_end or ( cells[i] - cells[i - 1] ) != ( cells[i + 1] - cells[i] ) ) route.push_back( centre_of( cells[i] ) );
  }
  return route;
}

} // namespace Game::PathFinding
