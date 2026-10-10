#ifndef SRC_COMPONENTS_ALTAR_SEGMENT_HPP__
#define SRC_COMPONENTS_ALTAR_SEGMENT_HPP__

namespace Game::Cmp::Altar
{

//! @brief Mainly used to distinguish between 16x16 block altar segments for the purpose of collision detection.
class Segment
{
public:
  //! @brief Construct a Segment with the given initial collision mask state.
  //! @param collision_mask Whether collision detection is enabled for this segment's sprite.
  Segment( bool collision_mask )
      : m_collision_mask( collision_mask )
  {
  }

  //! @brief Whether collision detection is currently enabled for this segment's sprite.
  //! @return bool True if solid (collidable).
  bool isCollisionMask() const { return m_collision_mask; }
  //! @brief Set whether collision detection is enabled for this segment's sprite.
  //! @param collision_mask The new collision mask state.
  void set_collision_mask( bool collision_mask ) { m_collision_mask = collision_mask; }

private:
  //! @brief Is collision detection enabled for this sprite.
  bool m_collision_mask{ true };
};

} // namespace Game::Cmp::Altar

#endif // SRC_COMPONENTS_ALTAR_SEGMENT_HPP__
