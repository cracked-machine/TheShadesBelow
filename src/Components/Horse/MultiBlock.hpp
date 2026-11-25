#ifndef SRC_COMPONENTS_HORSE_MULTIBLOCK_HPP__
#define SRC_COMPONENTS_HORSE_MULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>

namespace Game::Cmp::Horse
{

//! @brief Multi-block sprite for an Horse, defined geometrically as a rectangle.
//! @details Can be used to find its Segment parts.
class MultiBlock : public sf::FloatRect
{
public:
  //! @brief Construct an Horse MultiBlock at `position` with the given `size`.
  //! @param position Top-left world position of the block's bounding rect.
  //! @param size Width/height of the block's bounding rect.
  MultiBlock( const sf::Vector2f &position, const sf::Vector2f &size )
      : sf::FloatRect( position, size )
  {
  }
};

} // namespace Game::Cmp::Horse

#endif // SRC_COMPONENTS_HORSE_MULTIBLOCK_HPP__
