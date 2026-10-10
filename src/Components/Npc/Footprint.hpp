#ifndef SRC_COMPONENTS_NPC_FOOTPRINT_HPP__
#define SRC_COMPONENTS_NPC_FOOTPRINT_HPP__

#include <Components/Position.hpp>
#include <Utils/Constants.hpp>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <ranges>
#include <span>
#include <vector>

// clang-format off
namespace Game::Sprites { class SpriteSheet; }
// clang-format on

namespace Game::Cmp::Npc
{

//! @brief The grid cells an NPC stands on, which is what pathfinding moves around the navmesh and what collides
//! with the player. A single-block NPC stands on its own cell. A multiblock NPC stands on the cells its sprite
//! sheet marks in `collision_mask` (e.g. the bottom row of a tall sprite, so the rest of it can overhang obstacles),
//! or on every cell if it has no mask. If the mask has no `true` cell the NPC does not collide: see collides().
//! @details Cells are stored as grid pos (column, row) within the sprite's grid. Every other member converts
//! them to world space, given the NPC's top-left world position (its Cmp::Position).
//! @note Built once, when the NPC is created, from its first sprite sheet, so every sprite sheet of an NPC
//! (e.g. one per walk direction) must share the same `grid_size` and `collision_mask`.
class Footprint
{
public:
  //! @brief Build the footprint from a sprite sheet's `grid_size` and `collision_mask`.
  //! @param ss The NPC's sprite sheet.
  explicit Footprint( const Sprites::SpriteSheet &ss );

  //! @brief The grid pos (column, row) of every cell in the footprint: one per `true` entry of the sprite sheet's
  //! `collision_mask`, in row-major order. `{0, 0}` is the NPC's top-left.
  //! @return Never empty. Index zero is always the anchor cell for pathfinding: A* paths are computed for that
  //! cell, and the other cells are checked for clearance relative to it. A non-colliding footprint holds only
  //! the top-left cell, so the NPC can still be pathfound.
  [[nodiscard]] std::span<const sf::Vector2i> cells() const { return m_cells; }

  //! @brief Whether the NPC collides with the player.
  //! @return false if the sprite sheet's `collision_mask` has no `true` cell.
  [[nodiscard]] bool collides() const { return m_collides; }

  //! @brief Get the world rect covering every cell of the footprint.
  //! @param npc_top_left The NPC's top-left world position.
  //! @return sf::FloatRect
  [[nodiscard]] sf::FloatRect bounds( sf::Vector2f npc_top_left ) const;

  //! @brief Get the world position of the pathfinding anchor cell.
  //! @param npc_top_left The NPC's top-left world position.
  //! @return A cell-sized position.
  [[nodiscard]] Cmp::Position anchor( sf::Vector2f npc_top_left ) const;

  //! @brief The inverse of anchor(): get the NPC's top-left world position from its anchor cell's.
  //! @param anchor_world_pos World position of the anchor cell, e.g. an A* path node.
  //! @return sf::Vector2f
  [[nodiscard]] sf::Vector2f top_left_for_anchor( sf::Vector2f anchor_world_pos ) const;

  //! @brief Get the world position of every cell of the footprint, anchor cell first.
  //! @param npc_top_left The NPC's top-left world position.
  //! @return A lazy range of cell-sized Cmp::Position; valid only while this Footprint is alive.
  [[nodiscard]] auto world_cells( sf::Vector2f npc_top_left ) const
  {
    return m_cells | std::views::transform( [npc_top_left]( sf::Vector2i cell )
    { return Cmp::Position( npc_top_left + to_pixels( cell ), Constants::kGridSizePxF ); } );
  }

private:
  //! @brief Convert a grid pos or grid size to pixels.
  [[nodiscard]] static sf::Vector2f to_pixels( sf::Vector2i grid )
  {
    return { static_cast<float>( grid.x ) * Constants::kGridSizePxF.x, static_cast<float>( grid.y ) * Constants::kGridSizePxF.y };
  }

  //! @brief Grid pos of every cell, anchor cell first. See cells().
  std::vector<sf::Vector2i> m_cells{ { 0, 0 } };

  //! @brief False if the sprite sheet's collision mask has no true cell. See collides().
  bool m_collides{ true };

  //! @brief The rect covering m_cells, in grid cells. Fixed for the life of the footprint.
  sf::IntRect m_grid_bounds{ { 0, 0 }, { 1, 1 } };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_FOOTPRINT_HPP__
