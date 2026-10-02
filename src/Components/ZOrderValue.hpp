#ifndef SRC_COMPONENTS_ZORDERVALUE_HPP__
#define SRC_COMPONENTS_ZORDERVALUE_HPP__

namespace Game::Cmp
{

//! @brief Controls draw ordering (e.g. y-sorted depth) for renderable/sprite entities.
class ZOrderValue
{
public:
  //! @brief Construct with an initial z-order value.
  //! @param z The initial z-order value.
  ZOrderValue( float z )
      : zOrder( z ) {};

  //! @brief Returns the current z-order value.
  [[nodiscard]] float get() const { return zOrder; }

  //! @brief Sets the z-order value.
  //! @param z The new z-order value.
  void set( float z ) { zOrder = z; }

private:
  //! @brief The z-order value used to determine draw order relative to other entities.
  float zOrder;
};

} // namespace Game::Cmp
#endif // SRC_COMPONENTS_ZORDERVALUE_HPP__
