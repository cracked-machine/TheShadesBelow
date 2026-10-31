#ifndef SRC_COMPONENTS_NPC_PRIEST_HPP__
#define SRC_COMPONENTS_NPC_PRIEST_HPP__

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Priest NPC (periodically emits shockwaves via
//! Cmp::Npc::ShockwaveTimer).
struct Priest
{
  sf::Time sfx_cooldown{ sf::Time::Zero };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_PRIEST_HPP__
