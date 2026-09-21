#ifndef SRC_COMPONENTS_RECTBOUNDS_HPP__
#define SRC_COMPONENTS_RECTBOUNDS_HPP__

#include <Components/Position.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include <Factory/SpriteFactory.hpp>
#include <Systems/BaseSystem.hpp>
#include <Utils/Constants.hpp>

namespace Game::Cmp
{

//! @brief  A wrapper class for sf::FloatRect that provides scaled bounds with position offset calculations.
//! @note   RectBounds manages a rectangular boundary with automatic scaling and position offsetting
//!         based on sprite dimensions. It provides convenient methods for updating position and size
//!         while maintaining the scaling factor and offset calculations.
//!         The position offset is calculated to center the bounds relative to sprite dimensions,
//!         taking into account the scale factor.
class RectBounds
{
public:
  //! @brief Which axis (or both) the bounds size/offset scaling is applied to.
  enum class ScaleAxis {
    //! Scale only the horizontal axis.
    X,
    //! Scale only the vertical axis.
    Y,
    //! Scale both axes.
    XY
  };

  //! @brief Create a scaled hitbox centered on a grid-aligned entity
  static RectBounds scaled( sf::Vector2f pos, sf::Vector2f size, float scale_factor, ScaleAxis scale_axis = ScaleAxis::XY )
  {
    return { pos, size, scale_factor, scale_axis };
  }

  //! @brief Create a scaled hitbox centered on a grid-aligned entity
  static RectBounds scaled( const Cmp::Position &pos, float scale_factor, ScaleAxis scale_axis = ScaleAxis::XY )
  {
    return { pos.position, pos.size, scale_factor, scale_axis };
  }

  //! @brief Create a scaled hitbox centered on a grid-aligned entity
  static RectBounds scaled( const sf::FloatRect &pos, float scale_factor, ScaleAxis scale_axis = ScaleAxis::XY )
  {
    return { pos.position, pos.size, scale_factor, scale_axis };
  }

  //! @brief Create bounds expanded outward by N tiles in all directions
  static RectBounds expanded( sf::Vector2f position, sf::Vector2f size, int tiles ) { return RectBounds( position, size, tiles ); }

  //! @brief Create bounds directly from a FloatRect, expanded outward by N tiles
  static RectBounds expanded( const sf::FloatRect &rect, int tiles ) { return RectBounds( rect.position, rect.size, tiles ); }

  //! @brief Create bounds directly from a Cmp::Position, expanded outward by N tiles
  static RectBounds expanded( const Cmp::Position &pos, int tiles ) { return RectBounds( pos.position, pos.size, tiles ); }

  //! @brief Constructs a RectBounds object with specified position, size, and scale factor.
  //! @note  Recommend you use the static helper classes
  //! @param pos The initial position vector for the bounds
  //! @param size The base size vector before scaling is applied
  //! @param ScaleFactor The scaling factor to apply to the size dimensions
  //! @param scale_scale_axis Whether to scale in x, y or both dimensions
  RectBounds( sf::Vector2f pos, sf::Vector2f size, float ScaleFactor, ScaleAxis scale_scale_axis = ScaleAxis::XY )
      : m_scale_factor( ScaleFactor ),
        m_scale_scale_axis( scale_scale_axis )
  {
    // calculate the bounds scale/offset for requested scale_axis
    switch ( m_scale_scale_axis )
    {
      case ScaleAxis::X:
        m_bounds.size.x = size.x * m_scale_factor;
        m_bounds.size.y = size.y * 1;
        m_bounds.position.x = pos.x - size.x * kPositionOffsetFactor;
        m_bounds.position.y = pos.y;
        break;
      case ScaleAxis::Y:
        m_bounds.size.y = size.y * m_scale_factor;
        m_bounds.size.x = size.x * 1;
        m_bounds.position.y = pos.y - size.y * kPositionOffsetFactor;
        m_bounds.position.x = pos.x;
        break;
      case ScaleAxis::XY:
      default:
        m_bounds.size = size * m_scale_factor;
        m_bounds.position = pos - size * kPositionOffsetFactor;
        break;
    }
  }

  //! @brief Constructs a RectBounds expanded outward by N tiles in all directions, no scale/offset applied
  //! @note  Recommend you use the static helper classes
  //! @param pos The initial position vector for the bounds
  //! @param size The base size vector before expansion is applied
  //! @param expand_tiles The number of tile layers to add in the expansion
  RectBounds( sf::Vector2f pos, sf::Vector2f size, int expand_tiles )
      : m_scale_factor( 0.f ),
        m_scale_scale_axis( ScaleAxis::XY )
  {
    float dx = Constants::kGridSizePxF.x * static_cast<float>( expand_tiles );
    float dy = Constants::kGridSizePxF.y * static_cast<float>( expand_tiles );
    m_bounds = sf::FloatRect( sf::Vector2f{ pos.x - dx, pos.y - dy }, sf::Vector2f{ size.x + ( dx * 2.f ), size.y + ( dy * 2.f ) } );
  }

  //! @brief Polymorphic destructor for derived classes
  virtual ~RectBounds() = default;

  //! @brief Sets the position of the rectangular bounds
  //! @note Updates the bounds position by applying an offset calculation based on the new position.
  //!       The offset is calculated using the default sprite dimensions and `kPositionOffsetFactor`.
  //! @param new_position The new position to set for the bounds
  void position( sf::Vector2f new_position ) { m_bounds.position = new_position - Constants::kGridSizePxF * kPositionOffsetFactor; }

  //! @brief Retrieves the position of the rectangular bounds.
  sf::Vector2f position() const { return m_bounds.position; }

  //! @brief Sets the size of the rectangular bounds.
  //! @note Updates the internal bounds size by applying the scale factor to the provided dimensions.
  //!       The actual size stored will be the new_size multiplied by the current scale factor.
  //! @param new_size
  void size( sf::Vector2f new_size ) { m_bounds.size = new_size * m_scale_factor; }

  //! @brief Retrieves the size of the rectangular bounds.
  sf::Vector2f size() const { return m_bounds.size; }

  //! @brief Checks for intersection with another rectangle.
  std::optional<sf::FloatRect> findIntersection( const sf::FloatRect &rectangle ) const { return m_bounds.findIntersection( rectangle ); }

  //! @brief Retrieves the underlying FloatRect bounds.
  sf::FloatRect getBounds() const { return m_bounds; }

  //! @brief Converts the bounds to a Cmp::Position with matching position and size.
  Cmp::Position get_pos_cmp() const { return { m_bounds.position, m_bounds.size }; }

private:
  //! @brief The rectangular bounds of the component in world coordinates.
  sf::FloatRect m_bounds;

  //! @brief Scale factor used to modify the size of the rectangular bounds.
  float m_scale_factor;

  //! @brief Offset value used to center objects based on scale factor.
  float kPositionOffsetFactor{ ( m_scale_factor / 2.f ) - 0.5f };

  //! @brief The scale_axis of scaling applied to the bounds.
  ScaleAxis m_scale_scale_axis;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_RECTBOUNDS_HPP__
