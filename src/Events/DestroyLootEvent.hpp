#ifndef SRC_EVENTS_DESTROYLOOTEVENT_HPP__
#define SRC_EVENTS_DESTROYLOOTEVENT_HPP__

#include <entt/entity/entity.hpp>

namespace Game::Events
{

//! @brief Remove a dropped loot entity without the player collecting it, e.g. when its source plant burns.
//! @note Consumed by LootSystem. Trigger this instead of calling Factory::Loot::destroy_loot_drop directly.
struct DestroyLootEvent
{
  //! @brief Construct a new DestroyLootEvent object
  //! @param loot_entity The Cmp::Loot entity to remove.
  explicit DestroyLootEvent( entt::entity loot_entity )
      : m_loot_entity( loot_entity )
  {
  }

  //! @brief The Cmp::Loot entity to remove.
  entt::entity m_loot_entity;
};

} // namespace Game::Events

#endif // SRC_EVENTS_DESTROYLOOTEVENT_HPP__
