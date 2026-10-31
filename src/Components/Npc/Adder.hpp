#ifndef SRC_COMPONENTS_NPC_ADDER_HPP__
#define SRC_COMPONENTS_NPC_ADDER_HPP__

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Adder NPC (a hostile NPC type).
struct Adder
{
  sf::Time sfx_cooldown{ sf::Time::Zero };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_SPIDER_HPP__
