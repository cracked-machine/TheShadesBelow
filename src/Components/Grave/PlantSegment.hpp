#ifndef SRC_COMPONENTS_PLANTSEGMENT_HPP__
#define SRC_COMPONENTS_PLANTSEGMENT_HPP__

namespace Game::Cmp
{

//! @brief Mainly used to distinguish between 16x16 block plant segments for the purpose of collision
//! detection.
class PlantSegment
{
public:
  //! @brief Construct a new PlantSegment object.
  //! @param collision_mask Whether collision detection is enabled for this segment.
  PlantSegment( bool collision_mask )
      : m_collision_mask( collision_mask )
  {
  }

  //! @brief Check whether collision detection is enabled for this segment.
  //! @return bool
  bool isCollisionMask() const { return m_collision_mask; }

  //! @brief Set whether collision detection is enabled for this segment.
  //! @param collision_mask true to enable collision detection.
  void set_collision_mask( bool collision_mask ) { m_collision_mask = collision_mask; }

private:
  //! @brief Is collision detection enabled for this segment.
  bool m_collision_mask{ true };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_PLANTSEGMENT_HPP__
