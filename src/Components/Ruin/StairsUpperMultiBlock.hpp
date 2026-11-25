#ifndef SRC_COMPONENTS_RUIN_STAIRSUPPERMULTIBLOCK_HPP__
#define SRC_COMPONENTS_RUIN_STAIRSUPPERMULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>


namespace Game::Cmp::Ruin
{

//! @brief Multi-block sprite for the ruin's upper staircase, defined geometrically as a rectangle spanning
//! all of its Cmp::Ruin::StairsSegment parts.
//! @note Used to find the staircase's segment parts, track overall activation state, and apply a movement
//! slowdown penalty to the player while standing on it (see RuinSystem).
class StairsUpperMultiBlock : public sf::FloatRect
{
public:
  //! @brief Construct a multi-block covering the given rectangle.
  //! @param position Top-left world position of the multi-block.
  //! @param size Pixel dimensions of the multi-block.
  StairsUpperMultiBlock( const sf::Vector2f &position, const sf::Vector2f &size )
      : sf::FloatRect( position, size )
  {
  }
};

} // namespace Game::Cmp::Ruin

#endif // SRC_COMPONENTS_RUIN_STAIRSUPPERMULTIBLOCK_HPP__
