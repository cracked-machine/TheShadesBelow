#ifndef SRC_COMPONENTS_NPC_ELDERMOTHER_HPP__
#define SRC_COMPONENTS_NPC_ELDERMOTHER_HPP__

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Elder Mother NPC (a hostile, multiblock NPC type).
struct ElderMother
{
  //! @brief Always true; presence of the component is what identifies the entity as an Elder Mother.
  bool eldermother{ true };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_ELDERMOTHER_HPP__
