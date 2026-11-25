#ifndef SRC_COMPONENTS_RUIN_STAIRSBALUSTRADEMULTIBLOCK_HPP__
#define SRC_COMPONENTS_RUIN_STAIRSBALUSTRADEMULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>


namespace Game::Cmp::Ruin
{

//! @brief Multi-block sprite for the ruin stairs balustrade (railing), defined geometrically as a rectangle
//! spanning all of its Cmp::Ruin::StairsSegment parts.
//! @note Used to find the balustrade's segment parts and track overall activation state.
class StairsBalustradeMultiBlock : public sf::FloatRect
{
public:
  //! @brief Construct a multi-block covering the given rectangle.
  //! @param position Top-left world position of the multi-block.
  //! @param size Pixel dimensions of the multi-block.
  StairsBalustradeMultiBlock( const sf::Vector2f &position, const sf::Vector2f &size )
      : sf::FloatRect( position, size )
  {
  }
};

} // namespace Game::Cmp::Ruin

#endif // SRC_COMPONENTS_RUIN_STAIRSBALUSTRADEMULTIBLOCK_HPP__
