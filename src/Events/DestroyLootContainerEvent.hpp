#ifndef SRC_EVENTS_DESTROYLOOTCONTAINEREVENT_HPP__
#define SRC_EVENTS_DESTROYLOOTCONTAINEREVENT_HPP__

#include <entt/entity/entity.hpp>

namespace Game::Events
{

//! @brief Break a Cmp::LootContainer (e.g. a pot), with a random chance of dropping loot where it stood.
//! @note Consumed by LootSystem, which updates the navmeshes, spawns the break particles and rolls the loot drop.
//! Trigger this instead of calling Factory::Loot::destroy_loot_container directly. Do not trigger it while iterating
//! a view containing Cmp::Position: the handler creates a loot drop entity with a Cmp::Position, so collect the
//! entities first and trigger the event after the loop.
class DestroyLootContainerEvent
{
public:
  //! @param loot_container_entity The loot container to break.
  explicit DestroyLootContainerEvent( entt::entity loot_container_entity )
      : m_loot_container_entity( loot_container_entity )
  {
  }
  entt::entity m_loot_container_entity;
};

} // namespace Game::Events

#endif // SRC_EVENTS_DESTROYLOOTCONTAINEREVENT_HPP__
