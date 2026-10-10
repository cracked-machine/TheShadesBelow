#include <Components/Npc/Footprint.hpp>
#include <Sprites/SpriteSheet.hpp>

#include <algorithm>
#include <spdlog/spdlog.h>

namespace Game::Cmp::Npc
{

Footprint::Footprint( const Sprites::SpriteSheet &ss )
{
  const auto grid = ss.get_grid_size();
  // the sprite sheet guarantees the mask is either empty (every cell collides) or has at least one entry per cell
  const auto &collision_mask = ss.collision_mask();

  m_cells.clear();
  for ( int gy = 0; gy < grid.y; ++gy )
  {
    for ( int gx = 0; gx < grid.x; ++gx )
    {
      if ( not collision_mask.empty() and not collision_mask[static_cast<std::size_t>( ( gy * grid.x ) + gx )] ) continue;
      m_cells.emplace_back( gx, gy );
    }
  }

  if ( m_cells.empty() )
  {
    // an all-false mask is valid: the NPC doesn't collide. Pathfinding still needs a cell to route.
    SPDLOG_WARN( "{} has no true entries in its collision_mask: NPC will not collide", ss.type().str() );
    m_cells.emplace_back( 0, 0 );
    m_collides = false;
  }

  sf::Vector2i min_cell = m_cells.front();
  sf::Vector2i max_cell = m_cells.front();
  for ( auto cell : m_cells )
  {
    min_cell = { std::min( min_cell.x, cell.x ), std::min( min_cell.y, cell.y ) };
    max_cell = { std::max( max_cell.x, cell.x ), std::max( max_cell.y, cell.y ) };
  }
  m_grid_bounds = sf::IntRect( min_cell, max_cell - min_cell + sf::Vector2i{ 1, 1 } );
}

sf::FloatRect Footprint::bounds( sf::Vector2f npc_top_left ) const
{
  return { npc_top_left + to_pixels( m_grid_bounds.position ), to_pixels( m_grid_bounds.size ) };
}

Cmp::Position Footprint::anchor( sf::Vector2f npc_top_left ) const
{
  return { npc_top_left + to_pixels( m_cells.front() ), Constants::kGridSizePxF };
}

sf::Vector2f Footprint::top_left_for_anchor( sf::Vector2f anchor_world_pos ) const { return anchor_world_pos - to_pixels( m_cells.front() ); }

} // namespace Game::Cmp::Npc
