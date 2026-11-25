#ifndef SRC_COMPONENTS_SPRING_SACREDSPRINGMULTIBLOCK_HPP__
#define SRC_COMPONENTS_SPRING_SACREDSPRINGMULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>


namespace Game::Cmp
{

//! @brief Represents the interior healing spring/fountain itself as a multi-block sprite defined
//! geometrically as a rectangle.
//! @details Can be used to find its HealingSpringSegment parts and track overall activation state.
class HealingSpringMultiBlock : public sf::FloatRect
{
public:
  //! @brief Construct a new HealingSpringMultiBlock object.
  //! @param position Top-left world position of the multi-block bounds.
  //! @param size Width/height of the multi-block bounds.
  HealingSpringMultiBlock( const sf::Vector2f &position, const sf::Vector2f &size )
      : sf::FloatRect( position, size )
  {
  }
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_SPRING_SACREDSPRINGMULTIBLOCK_HPP__
