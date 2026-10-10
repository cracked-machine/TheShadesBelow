#ifndef SRC_COMPONENTS_POSITION_HPP__
#define SRC_COMPONENTS_POSITION_HPP__

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

namespace Game::Cmp
{

//! @brief Component used to contain a grid position (in pixels). Also contains the size of the grid square (usually 16x16px)
class Position : public sf::FloatRect
{
public:
  //! @brief Construct a grid position at `pos` with grid square size `size`, both in pixels.
  //! @param pos The grid position, in pixels.
  //! @param size The grid square size, in pixels.
  Position( const sf::Vector2f &pos, const sf::Vector2f &size )
      : sf::FloatRect( pos, size )
  {
  }

  //! @brief Returns the x coordinate of the position, in pixels.
  float x() const { return position.x; }

  //! @brief Returns the y coordinate of the position, in pixels.
  float y() const { return position.y; }

  //! @brief Adds the positions of two Position components, keeping this instance's size.
  //! @param rhs The position to add.
  //! @return Position The summed position with this instance's size.
  Position operator+( const Position &rhs ) const { return { position + rhs.position, size }; }
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_POSITION_HPP__
