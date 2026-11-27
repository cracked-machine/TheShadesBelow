#ifndef SRC_COMPONENTS_LEVELCOMPLETE_TABLEMULTIBLOCK_HPP__
#define SRC_COMPONENTS_LEVELCOMPLETE_TABLEMULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>

namespace Game::Cmp::LevelComplete
{

//! @brief Multi-block sprite for the level complete scene's tally table, defined geometrically as a rectangle spanning
//! all of its Cmp::LevelComplete::TableSegment parts. Barrels and inventory items are drawn on it as they are counted.
class TableMultiBlock : public sf::FloatRect
{
public:
  //! @brief Construct a multi-block covering the given rectangle.
  //! @param position Top-left world position of the multi-block.
  //! @param size Pixel dimensions of the multi-block.
  TableMultiBlock( const sf::Vector2f &position, const sf::Vector2f &size )
      : sf::FloatRect( position, size )
  {
  }
};

} // namespace Game::Cmp::LevelComplete

#endif // SRC_COMPONENTS_LEVELCOMPLETE_TABLEMULTIBLOCK_HPP__
