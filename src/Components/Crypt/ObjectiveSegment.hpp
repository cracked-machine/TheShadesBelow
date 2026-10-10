#ifndef SRC_COMPONENTS_CRYPT_OBJECTIVESEGMENT_HPP__
#define SRC_COMPONENTS_CRYPT_OBJECTIVESEGMENT_HPP__

namespace Game::Cmp::Crypt
{

//! @brief Marks a single 16x16 px block of a Cmp::Crypt::ObjectiveMultiBlock, mainly used to distinguish
//! between individual objective segments for the purpose of collision detection.
class ObjectiveSegment
{
public:
  //! @brief Construct a new segment.
  //! @param collision_mask Initial collision-enabled state for this segment.
  ObjectiveSegment( bool collision_mask )
      : m_collision_mask( collision_mask )
  {
  }

  //! @brief Get whether collision detection is currently enabled for this segment.
  //! @return bool
  bool isCollisionMask() const { return m_collision_mask; }
  //! @brief Set whether collision detection is enabled for this segment.
  //! @param collision_mask
  void set_collision_mask( bool collision_mask ) { m_collision_mask = collision_mask; }

private:
  //! @brief Is collision detection enabled for this sprite.
  bool m_collision_mask{ true };
};

} // namespace Game::Cmp::Crypt

#endif // SRC_COMPONENTS_CRYPT_OBJECTIVESEGMENT_HPP__
