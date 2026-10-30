#ifndef SRC_COMPONENTS_TELEPORTANIM_HPP__
#define SRC_COMPONENTS_TELEPORTANIM_HPP__

namespace Game::Cmp
{

//! @brief Marker component tagging a one-shot teleport animation entity, so TeleportSystem can destroy it once finished.
struct TeleportAnim
{
  //! @brief Always true; presence of the component is what identifies the entity as a teleport animation.
  bool teleport_anim{ true };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_TELEPORTANIM_HPP__
