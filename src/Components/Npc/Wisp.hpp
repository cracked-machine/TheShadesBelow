#ifndef SRC_COMPONENTS_NPC_WISP_HPP__
#define SRC_COMPONENTS_NPC_WISP_HPP__

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Wisp NPC (a friendly NPC type).
struct Wisp
{
  sf::Time sfx_cooldown{ sf::Time::Zero };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_WISP_HPP__
