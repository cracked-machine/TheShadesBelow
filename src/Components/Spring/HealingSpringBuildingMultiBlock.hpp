#ifndef SRC_COMPONENTS_SPRING_SACREDSPRINGBUILDINGMULTIBLOCK_HPP__
#define SRC_COMPONENTS_SPRING_SACREDSPRINGBUILDINGMULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>


namespace Game::Cmp
{

//! @brief Represents the exterior healing spring building as a multi-block sprite defined geometrically
//! as a rectangle.
//! @details Can be used to find its HealingSpringBuildingSegment parts and track overall activation state.
class HealingSpringBuildingMultiBlock : public sf::FloatRect
{
public:
  //! @brief Construct a new HealingSpringBuildingMultiBlock object.
  //! @param position Top-left world position of the multi-block bounds.
  //! @param size Width/height of the multi-block bounds.
  HealingSpringBuildingMultiBlock( const sf::Vector2f &position, const sf::Vector2f &size )
      : sf::FloatRect( position, size )
  {
  }
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_SPRING_SACREDSPRINGBUILDINGMULTIBLOCK_HPP__
