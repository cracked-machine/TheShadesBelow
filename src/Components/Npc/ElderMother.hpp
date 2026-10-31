#ifndef SRC_COMPONENTS_NPC_ELDERMOTHER_HPP__
#define SRC_COMPONENTS_NPC_ELDERMOTHER_HPP__

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Elder Mother NPC (a hostile, multiblock NPC type).
struct ElderMother
{
  sf::Time sfx_cooldown{ sf::Time::Zero };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_ELDERMOTHER_HPP__
