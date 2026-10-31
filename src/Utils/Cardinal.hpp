#ifndef SRC_UTILS_CARDINAL_HPP__
#define SRC_UTILS_CARDINAL_HPP__

#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>

namespace Game::Utils
{

//! @brief A cyclable cardinal direction (East/South/West/North). Range-based-for iterable via
//! `for ( auto c : Cardinal() )` (visits each direction exactly once) and independently steppable
//! via operator++ (wraps forever: North -> East -> South -> ...). These two are deliberately
//! decoupled — the range iterator has its own non-wrapping counter — because a wrapping operator++
//! driving range termination would never reach an end sentinel and loop forever.
class Cardinal
{
public:
  //! @brief The four cardinal directions, ordered clockwise starting from East.
  enum Value : int {
    //! @brief Facing right, unit vector {1,0}
    East = 0,
    //! @brief Facing down, unit vector {0,1}
    South = 1,
    //! @brief Facing left, unit vector {-1,0}
    West = 2,
    //! @brief Facing up, unit vector {0,-1}
    North = 3
  };

  //! @brief Number of distinct Cardinal values.
  static constexpr int Count = 4;

  //! @brief Construct a Cardinal, defaulting to East.
  //! @param value The initial direction.
  constexpr Cardinal( Value value = East )
      : m_value( value )
  {
  }

  //! @brief Step to the next cardinal direction (wraps East -> South -> West -> North -> East)
  constexpr Cardinal &operator++()
  {
    m_value = static_cast<Value>( ( static_cast<int>( m_value ) + 1 ) % Count );
    return *this;
  }

  //! @brief Compare two Cardinal directions for equality.
  //! @param rhs The direction to compare against.
  //! @return true if both hold the same Value.
  constexpr bool operator==( const Cardinal &rhs ) const = default;

  //! @brief Get the underlying direction value.
  //! @return Value The current direction.
  [[nodiscard]] constexpr Value value() const { return m_value; }

  //! @brief Convert to the corresponding unit vector: {1,0} right, {0,1} down, {-1,0} left, {0,-1} up
  [[nodiscard]] constexpr sf::Vector2f vector() const
  {
    switch ( m_value )
    {
      case East:
        return { 1.f, 0.f };
      case South:
        return { 0.f, 1.f };
      case West:
        return { -1.f, 0.f };
      case North:
        return { 0.f, -1.f };
      default:
        return { 0.f, 0.f };
    }
  }

  //! @brief The rotation of this direction, clockwise from East as SFML rotates with y pointing down.
  //! @return sf::Angle 0 deg East, 90 deg South, 180 deg West, 270 deg North
  [[nodiscard]] constexpr sf::Angle angle() const { return sf::degrees( static_cast<float>( m_value ) * 90.f ); }

  //! @brief The cardinal direction of a vector's larger axis; a tie resolves to the horizontal axis.
  //! @param v The vector to classify.
  //! @return Cardinal East/West when |x| >= |y|, otherwise South/North.
  [[nodiscard]] static constexpr Cardinal dominant( sf::Vector2f v )
  {
    const float abs_x = v.x < 0.f ? -v.x : v.x;
    const float abs_y = v.y < 0.f ? -v.y : v.y;
    if ( abs_x >= abs_y ) return v.x >= 0.f ? East : West;
    return v.y >= 0.f ? South : North;
  }

  //! @brief Minimal iterator over the 4 Cardinal values, using a plain non-wrapping counter —
  //! independent of Cardinal::operator++'s cyclic semantics.
  class Iterator
  {
  public:
    //! @brief Construct an iterator at the given non-wrapping index.
    //! @param index Index in [0, Cardinal::Count].
    constexpr explicit Iterator( int index )
        : m_index( index )
    {
    }

    //! @brief Dereference to the Cardinal at the current index.
    //! @return Cardinal The direction corresponding to the current index.
    constexpr Cardinal operator*() const { return { static_cast<Value>( m_index ) }; }

    //! @brief Advance to the next index (does not wrap).
    //! @return Iterator& Reference to this iterator, now advanced.
    constexpr Iterator &operator++()
    {
      ++m_index;
      return *this;
    }

    //! @brief Compare two iterators for inequality, based on their index.
    //! @param rhs The iterator to compare against.
    //! @return true if the indices differ.
    constexpr bool operator!=( const Iterator &rhs ) const { return m_index != rhs.m_index; }

  private:
    //! @brief Current position in the range [0, Cardinal::Count].
    int m_index;
  };

  //! @brief Range-for begin iterator — enables `for ( auto c : Cardinal() )`.
  //! @return Iterator Iterator at index 0 (East).
  static constexpr Iterator begin() { return Iterator( 0 ); }

  //! @brief Range-for end sentinel — enables `for ( auto c : Cardinal() )`.
  //! @return Iterator Iterator one past the last direction.
  static constexpr Iterator end() { return Iterator( Count ); }

private:
  //! @brief The currently held direction.
  Value m_value;
};

} // namespace Game::Utils

#endif // SRC_UTILS_CARDINAL_HPP__
