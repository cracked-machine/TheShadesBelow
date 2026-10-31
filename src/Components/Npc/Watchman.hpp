#ifndef SRC_COMPONENTS_NPC_WATCHMAN_HPP__
#define SRC_COMPONENTS_NPC_WATCHMAN_HPP__

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Watchman NPC (a hostile NPC type; see also
//! Cmp::Npc::WatchmanSearchlight for its per-entity searchlight/gunfire state).
struct Watchman
{
  sf::Time sfx_cooldown{ sf::Time::Zero };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_WATCHMAN_HPP__
