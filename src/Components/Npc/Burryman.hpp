#ifndef SRC_COMPONENTS_NPC_BURRYMAN_HPP__
#define SRC_COMPONENTS_NPC_BURRYMAN_HPP__

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Burryman NPC (a hostile NPC type).
struct Burryman
{
  sf::Time sfx_cooldown{ sf::Time::Zero };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_BURRYMAN_HPP__
