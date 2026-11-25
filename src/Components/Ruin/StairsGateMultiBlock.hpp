#ifndef SRC_COMPONENTS_RUIN_STAIRSGATEMULTIBLOCK_HPP__
#define SRC_COMPONENTS_RUIN_STAIRSGATEMULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>


namespace Game::Cmp::Ruin
{

//! @brief This gate blocks the player from accessing the stairs until the ruinlowerscene puzzle is completed.
class StairsGateMultiBlock : public sf::FloatRect
{
public:
  //! @brief Construct a multi-block covering the given rectangle.
  //! @param position Top-left world position of the multi-block.
  //! @param size Pixel dimensions of the multi-block.
  StairsGateMultiBlock( const sf::Vector2f &position, const sf::Vector2f &size )
      : sf::FloatRect( position, size )
  {
  }
};

} // namespace Game::Cmp::Ruin

#endif // SRC_COMPONENTS_RUIN_STAIRSGATEMULTIBLOCK_HPP__
