#ifndef SRC_COMPONENTS_NPC_TARGET_HPP__
#define SRC_COMPONENTS_NPC_TARGET_HPP__


namespace Game::Cmp::Npc
{
//! @brief Used for NPC pathfinding if their target is not the player.
//! @note Add this component to an entity that already has a Cmp::Position to mark it as an NPC target.
struct Target
{
  //! @brief Construct a target reference.
  //! @param id The entity being targeted (must have a Cmp::Position).
  Target( entt::entity id )
      : id( id )
  {
  }

  //! @brief The entity being targeted, or entt::null if none.
  entt::entity id{ entt::null };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_TARGET_HPP__
