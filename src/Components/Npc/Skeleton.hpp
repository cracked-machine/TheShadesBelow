#ifndef SRC_COMPONENTS_NPC_SKELETON_HPP__
#define SRC_COMPONENTS_NPC_SKELETON_HPP__

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Skeleton NPC (a hostile NPC type).
struct Skeleton
{
  sf::Time sfx_cooldown{ sf::Time::Zero };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_SKELETON_HPP__
