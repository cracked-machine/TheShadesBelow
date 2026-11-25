#ifndef SRC_SPRITES_SHOCKWAVE_HPP__
#define SRC_SPRITES_SHOCKWAVE_HPP__

#include <Sprites/CircleSegment.hpp>

namespace Game::Sprites
{

//! @brief Represents a shockwave as a collection of sprite circle segments.
//! @details As the shockwave hits obstacles it will split into segments.
class Shockwave
{
public:
  //! @brief Circle segments contained in this Shockwave
  using CircleSegments = std::vector<CircleSegment>;

  //! @brief Construct a new Shockwave Sprite object
  //! @param pos
  //! @param points_per_segment
  Shockwave( sf::Vector2f pos, int points_per_segment );

  //! @brief Get the Points Per Segment object
  //! @return int
  [[nodiscard]] int get_points_per_segment() const { return m_points_per_segment; }

  //! @brief Set the Position object
  //! @param pos
  void set_position( sf::Vector2f pos )
  {
    m_position = pos;
    invalidate_all_segments();
  }

  //! @brief Get the Position object
  //! @return sf::Vector2f
  sf::Vector2f get_position() const { return m_position; }

  //! @brief Set the Radius object
  //! @param radius
  void set_radius( float radius )
  {
    m_radius = radius;
    invalidate_all_segments();
  }

  //! @brief Get the Radius object
  //! @return float
  [[nodiscard]] float get_radius() const { return m_radius; }

  //! @brief Set the Outline Color object
  //! @param color
  void set_outline_color( sf::Color color )
  {
    m_outline_color = color;
    invalidate_all_segments();
  }

  //! @brief Get the Outline Color object
  //! @return sf::Color
  [[nodiscard]] sf::Color get_outline_color() const { return m_outline_color; }

  //! @brief Set the Outline Thickness object
  //! @param thickness
  void set_outline_thickness( float thickness )
  {
    m_outline_thickness = thickness;
    invalidate_all_segments();
  }

  //! @brief Get the Segments object
  //! @return const CircleSegments&
  [[nodiscard]] const CircleSegments &get_segments() const { return m_segments; }

  //! @brief Set the Segments object
  //! @param segments
  void set_segments( CircleSegments &&segments )
  {
    m_segments = std::move( segments );
    invalidate_all_segments();
  }

  //! @brief Get the Outline Thickness object
  //! @return float
  [[nodiscard]] float get_outline_thickness() const { return m_outline_thickness; }

  //! @brief Check if a point intersects with the circle outline
  //! @param point
  //! @return true
  //! @return false
  [[nodiscard]] bool intersects_with_point( sf::Vector2f point ) const;

  //! @brief Check if a rectangle intersects with any part of the circle outline
  //! @param rect
  //! @return true
  //! @return false
  [[nodiscard]] bool intersects_with_rect( const sf::FloatRect &rect ) const;

  //! @brief Get a list of all visible segments (for debugging or other purposes)
  //! @return CircleSegments
  [[nodiscard]] CircleSegments get_visible_segments() const;

private:
  //! @brief Center position of the shockwave circle.
  sf::Vector2f m_position;
  //! @brief Radius of the shockwave circle.
  float m_radius;
  //! @brief Outline color applied to all segments.
  sf::Color m_outline_color;
  //! @brief Outline thickness applied to all segments.
  float m_outline_thickness;
  //! @brief The angular segments that make up the shockwave circle.
  CircleSegments m_segments;
  //! @brief Number of points used to tessellate each segment.
  int m_points_per_segment;

  //! @brief Invalidate all segment caches
  void invalidate_all_segments();
};

} // namespace Game::Sprites

#endif // SRC_SPRITES_SHOCKWAVE_HPP__
