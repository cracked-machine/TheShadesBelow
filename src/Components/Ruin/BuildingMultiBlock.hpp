#ifndef SRC_COMPONENTS_RUIN_BUILDINGMULTIBLOCK_HPP__
#define SRC_COMPONENTS_RUIN_BUILDINGMULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>


namespace Game::Cmp::Ruin
{

//! @brief Multi-block sprite for a ruin building, defined geometrically as a rectangle spanning
//! all of its Cmp::Ruin::BuildingSegment parts.
//! @note Used to find the building's segment parts and track overall activation state.
class BuildingMultiBlock : public sf::FloatRect
{
public:
  //! @brief Construct a multi-block covering the given rectangle.
  //! @param position Top-left world position of the multi-block.
  //! @param size Pixel dimensions of the multi-block.
  BuildingMultiBlock( const sf::Vector2f &position, const sf::Vector2f &size )
      : sf::FloatRect( position, size )
  {
  }
};

} // namespace Game::Cmp::Ruin

#endif // SRC_COMPONENTS_RUIN_BUILDINGMULTIBLOCK_HPP__
