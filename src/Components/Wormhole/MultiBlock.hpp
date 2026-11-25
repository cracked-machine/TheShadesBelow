#ifndef SRC_COMPONENTS_WORMHOLE_MULTIBLOCK_HPP__
#define SRC_COMPONENTS_WORMHOLE_MULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>


namespace Game::Cmp::Wormhole
{

//! @brief Multi-block sprite for a wormhole, defined geometrically as a rectangle.
//! @details Also tracks activation of its constituent sprites and overall activation state.
class MultiBlock : public sf::FloatRect
{
public:
  //! @brief Construct a wormhole MultiBlock at `position` with the given `size`.
  //! @param position Top-left world position of the block's bounding rect.
  //! @param size Width/height of the block's bounding rect.
  MultiBlock( const sf::Vector2f &position, const sf::Vector2f &size )
      : sf::FloatRect( position, size )
  {
  }
};

} // namespace Game::Cmp::Wormhole

#endif // SRC_COMPONENTS_WORMHOLE_MULTIBLOCK_HPP__
