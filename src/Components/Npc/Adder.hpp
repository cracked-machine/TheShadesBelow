#ifndef SRC_COMPONENTS_NPC_ADDER_HPP__
#define SRC_COMPONENTS_NPC_ADDER_HPP__

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Spider NPC (a hostile NPC type).
struct Adder
{
  //! @brief Always true; presence of the component is what identifies the entity as a Spider.
  bool adder{ true };
  sf::Time sfx_cooldown{ sf::Time::Zero };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_SPIDER_HPP__
