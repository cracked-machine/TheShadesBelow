#ifndef SRC_COMPONENTS_SPAWNAREA_HPP__
#define SRC_COMPONENTS_SPAWNAREA_HPP__

namespace Game::Cmp
{

//! @brief Marks a tile/entity as a valid spawn location for procedural level generation.
class SpawnArea
{
public:
  //! @brief Construct a spawn area marker.
  //! @param collision_mask Whether collision detection is enabled for this spawn area.
  SpawnArea( bool collision_mask )
      : m_collision_mask( collision_mask )
  {
  }

  //! @brief Returns whether collision detection is enabled for this spawn area.
  bool isCollisionMask() const { return m_collision_mask; }

private:
  //! @brief Is collision detection enabled for this sprite.
  bool m_collision_mask{ true };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_SPAWNAREA_HPP__
