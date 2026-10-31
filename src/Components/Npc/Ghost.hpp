#ifndef SRC_COMPONENTS_NPC_GHOST_HPP__
#define SRC_COMPONENTS_NPC_GHOST_HPP__

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Ghost NPC (a hostile NPC type).
struct Ghost
{
  sf::Time sfx_cooldown{ sf::Time::Zero };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_GHOST_HPP__
