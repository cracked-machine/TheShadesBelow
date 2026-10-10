#ifndef SRC_COMPONENTS_RUIN_HEXAGRAMSEGMENT_HPP__
#define SRC_COMPONENTS_RUIN_HEXAGRAMSEGMENT_HPP__

#include <SFML/Graphics/RectangleShape.hpp>

#include <Components/Obstacle.hpp>

namespace Game::Cmp::Ruin
{

//! @brief Marks a single 16x16 px block of a Cmp::Ruin::HexagramMultiBlock, mainly used to distinguish
//! between individual hexagram segments for the purpose of collision detection.
class HexagramSegment
{
public:
  //! @brief Construct a new segment.
  //! @param collision_mask Initial collision-enabled state for this segment.
  HexagramSegment( bool collision_mask )
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

} // namespace Game::Cmp::Ruin

#endif // SRC_COMPONENTS_RUIN_HEXAGRAMSEGMENT_HPP__
