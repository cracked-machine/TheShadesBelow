#include <Components/Npc/Footprint.hpp>
#include <Sprites/SpriteSheet.hpp>

#include <algorithm>
#include <functional>

namespace Game::Cmp::Npc
{

Footprint::Footprint( const Sprites::SpriteSheet &ss )
{
  auto grid = ss.get_grid_size();
  if ( grid == sf::Vector2i{ 1, 1 } ) return;

  const auto &solid_mask = ss.solid_mask();
  const auto grid_cell_count = grid.x * grid.y;
  // a missing, short or all-false mask means the NPC stands on every cell it covers
  const bool use_mask = solid_mask.size() >= static_cast<size_t>( grid_cell_count ) and
                        std::ranges::any_of( solid_mask | std::views::take( grid_cell_count ), std::identity{} );

  m_cells.clear();
  for ( int gy = 0; gy < grid.y; ++gy )
  {
    for ( int gx = 0; gx < grid.x; ++gx )
    {
      if ( use_mask and not solid_mask[( gy * grid.x ) + gx] ) continue;
      m_cells.emplace_back( gx, gy );
    }
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
