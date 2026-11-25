
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>

namespace Game::Utils::Maths
{

//! @brief Get the Manhattan Distance between two positions.
//! Creates a grid-like distance metric:
//! ┌────┬────┬────┐
//! │ 2  │ 1  │ 2  │  Sum of absolute differences
//! ├────┼────┼────┤  Moves only horizontal/vertical
//! │ 1  │ 0  │ 1  │  (Like a taxi in a city grid)
//! ├────┼────┼────┤
//! │ 2  │ 1  │ 2  │
//! └────┴────┴────┘
//! @note NPCs will be unable to "see" around corners with this distance metric.
//! @param posA The first position.
//! @param posB The second position.
//! @return unsigned int The Manhattan distance.
template <typename T>
constexpr T getManhattanDistance( sf::Vector2<T> posA, sf::Vector2<T> posB )
{
  return std::abs( posA.x - posB.x ) + std::abs( posA.y - posB.y );
}

template int getManhattanDistance<int>( sf::Vector2i, sf::Vector2i );
template float getManhattanDistance<float>( sf::Vector2f, sf::Vector2f );

//! @brief Get the Chebyshev Distance between two positions.
//! Creates an equal-cost distance metric for all 8 directions:
//! ┌────┬────┬────┐
//! │ 1  │ 1  │ 1  │  Maximum of x or y distance
//! ├────┼────┼────┤  All 8 neighbors are distance 1
//! │ 1  │ 0  │ 1  │  (Like a chess king's move)
//! ├────┼────┼────┤
//! │ 1  │ 1  │ 1  │
//! └────┴────┴────┘
//! @note NPC pathfinding will pick randomly and appear to zig-zag
//! @param posA The first position.
//! @param posB The second position.
//! @return unsigned int The Chebyshev distance.
template <typename T>
constexpr T getChebyshevDistance( sf::Vector2<T> posA, sf::Vector2<T> posB )
{
  return std::max( std::abs( posA.x - posB.x ), std::abs( posA.y - posB.y ) );
}

template int getChebyshevDistance<int>( sf::Vector2<int>, sf::Vector2<int> );
template float getChebyshevDistance<float>( sf::Vector2<float>, sf::Vector2<float> );

//! @brief Get the Euclidean Distance between two positions.
//! Creates a straight-line distance metric:
//! ┌─────┬─────┬─────┐
//! │ 1.4 │ 1.0 │ 1.4 │  Straight-line distance
//! ├─────┼─────┼─────┤  Diagonal = √2 ≈ 1.414
//! │ 1.0 │ 0.0 │ 1.0 │  (Standard geometric distance)
//! ├─────┼─────┼─────┤
//! │ 1.4 │ 1.0 │ 1.4 │
//! └─────┴─────┴─────┘
//! @note NPCs pathfinding will be able to navigate around obstacles
//! @param posA The first position.
//! @param posB The second position.
//! @return unsigned int The Euclidean distance.
template <typename T>
constexpr T getEuclideanDistance( sf::Vector2<T> posA, sf::Vector2<T> posB )
{
  T dx = posA.x - posB.x;
  T dy = posA.y - posB.y;
  return static_cast<T>( std::sqrt( ( dx * dx ) + ( dy * dy ) ) );
}

template int getEuclideanDistance<int>( sf::Vector2<int>, sf::Vector2<int> );
template float getEuclideanDistance<float>( sf::Vector2<float>, sf::Vector2<float> );

float normalizeAngle( float angle )
{
  angle = std::fmod( angle, 2.0f * std::numbers::pi );
  return angle < 0 ? angle + ( 2.0f * std::numbers::pi ) : angle;
}

std::optional<sf::Vector2f> normalized( sf::Vector2f v )
{
  float len = v.length();
  if ( len < 0.1f ) return std::nullopt;
  return v / len;
}

[[nodiscard]] std::optional<sf::Angle> angle( sf::Vector2f v )
{
  if ( v.x == 0.f && v.y == 0.f ) return std::nullopt;
  return v.angle();
}

//! @brief Create a thick line rect object
//! @example `m_window.draw( Utils::Maths::thick_line_rect( source_pos, corner, color, thickness ) );`
//! @param start
//! @param end
//! @param color
//! @param thickness
//! @return sf::RectangleShape
sf::RectangleShape thick_line_rect( sf::Vector2f start, sf::Vector2f end, sf::Color color, float thickness )
{
  sf::Vector2f direction = end - start;
  float length = direction.length();

  auto maybe_angle = Utils::Maths::angle( direction );
  if ( not maybe_angle.has_value() ) return sf::RectangleShape{};
  sf::Angle angle = maybe_angle.value();

  sf::RectangleShape line( { length, thickness } );
  line.setPosition( start );
  line.setOrigin( { 0.f, thickness / 2.f } ); // center vertically
  line.setRotation( angle );
  line.setFillColor( color );
  return line;
};

//! @brief Create a thick line quad object
//! @example `m_window.draw( Utils::Maths::thick_line_quad(quad.data(), quad.size(), sf::PrimitiveType::TriangleStrip) );`
//! @param start
//! @param end
//! @param color
//! @param thickness
//! @return std::array<sf::Vertex, 4>
std::array<sf::Vertex, 4> thick_line_quad( sf::Vector2f start, sf::Vector2f end, sf::Color color, float thickness )
{
  sf::Vector2f direction = ( end - start ).normalized();
  sf::Vector2f perpendicular{ -direction.y, direction.x };
  sf::Vector2f offset = perpendicular * ( thickness / 2.f );

  std::array<sf::Vertex, 4> quad = { sf::Vertex{ start - offset, color }, sf::Vertex{ start + offset, color }, sf::Vertex{ end - offset, color },
                                     sf::Vertex{ end + offset, color } };
  return quad;
};

uint8_t to_percent( float max_value, uint8_t convert )
{
  auto converted = std::round( ( max_value / 100 ) * convert );
  return static_cast<uint8_t>( converted );
}

float exp_decay( float current, float target, float rate, float dt )
{
  return current + ( ( target - current ) * ( 1.0f - std::exp( -rate * dt ) ) );
}

bool segment_intersects_rect( sf::Vector2f a, sf::Vector2f b, sf::FloatRect rect )
{
  const float rect_min_x = rect.position.x;
  const float rect_min_y = rect.position.y;
  const float rect_max_x = rect.position.x + rect.size.x;
  const float rect_max_y = rect.position.y + rect.size.y;

  const sf::Vector2f d = b - a;
  float t_min = 0.f;
  float t_max = 1.f;

  if ( std::abs( d.x ) < 1e-6f )
  {
    if ( a.x < rect_min_x or a.x > rect_max_x ) return false;
  }
  else
  {
    float t1 = ( rect_min_x - a.x ) / d.x;
    float t2 = ( rect_max_x - a.x ) / d.x;
    if ( t1 > t2 ) std::swap( t1, t2 );
    t_min = std::max( t_min, t1 );
    t_max = std::min( t_max, t2 );
    if ( t_min > t_max ) return false;
  }

  if ( std::abs( d.y ) < 1e-6f )
  {
    if ( a.y < rect_min_y or a.y > rect_max_y ) return false;
  }
  else
  {
    float t1 = ( rect_min_y - a.y ) / d.y;
    float t2 = ( rect_max_y - a.y ) / d.y;
    if ( t1 > t2 ) std::swap( t1, t2 );
    t_min = std::max( t_min, t1 );
    t_max = std::min( t_max, t2 );
    if ( t_min > t_max ) return false;
  }

  return true;
}

bool is_point_in_cone( sf::Vector2f apex, sf::Vector2f direction, float half_angle_radians, float length, sf::Vector2f point )
{
  const sf::Vector2f to_point = point - apex;
  if ( to_point.length() >= length ) return false;

  const auto n_to_point = normalized( to_point );
  const auto n_direction = normalized( direction );
  if ( not n_to_point.has_value() or not n_direction.has_value() ) return false;

  return n_to_point->dot( *n_direction ) > std::cos( half_angle_radians );
}

} // namespace Game::Utils::Maths
