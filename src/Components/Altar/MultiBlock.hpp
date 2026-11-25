#ifndef SRC_COMPONENTS_ALTAR_MULTIBLOCK_HPP__
#define SRC_COMPONENTS_ALTAR_MULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>

#include <Components/Altar/Kind.hpp>

namespace Game::Cmp::Altar
{

//! @brief Multi-block sprite for an altar, defined geometrically as a rectangle.
//! @details Can be used to find its Segment parts.
class MultiBlock : public sf::FloatRect
{
public:
  //! @brief Construct an altar MultiBlock at `position` with the given `size`.
  //! @param position Top-left world position of the block's bounding rect.
  //! @param size Width/height of the block's bounding rect.
  MultiBlock( const sf::Vector2f &position, const sf::Vector2f &size )
      : sf::FloatRect( position, size )
  {
  }

  //! @brief Set which kind of altar this is. Defaults to Kind::GRAVEYARD.
  //! @param kind The altar kind.
  void set_kind( Kind kind ) { m_kind = kind; }
  //! @brief Get which kind of altar this is, used to pick the item's offering reward for this altar.
  //! @return Kind The altar kind.
  [[nodiscard]] Kind get_kind() const { return m_kind; }

private:
  //! @brief Which kind of altar this is.
  Kind m_kind{ Kind::GRAVEYARD };
};

} // namespace Game::Cmp::Altar

#endif // SRC_COMPONENTS_ALTAR_MULTIBLOCK_HPP__
