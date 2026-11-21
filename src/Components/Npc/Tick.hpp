#ifndef SRC_COMPONENTS_NPC_TICK_HPP__
#define SRC_COMPONENTS_NPC_TICK_HPP__

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Tick NPC (a hostile NPC that can crawl out of dug-up grass).
struct Tick
{
  //! @brief Always true; presence of the component is what identifies the entity as a Tick.
  bool tick{ true };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_TICK_HPP__
