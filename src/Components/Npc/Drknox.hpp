#ifndef SRC_COMPONENTS_NPC_DRKNOX_HPP__
#define SRC_COMPONENTS_NPC_DRKNOX_HPP__

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Drknox NPC (a friendly NPC type).
struct Drknox
{
  sf::Time sfx_cooldown{ sf::Time::Zero };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_DRKNOX_HPP__
