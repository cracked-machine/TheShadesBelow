#ifndef SRC_EVENTS_DESTROYITEMEVENT_HPP__
#define SRC_EVENTS_DESTROYITEMEVENT_HPP__

#include <Components/Position.hpp>
#include <entt/entity/entity.hpp>

namespace Game::Events
{

//! @brief Remove a world item.
struct DestroyItemEvent
{
  //! @brief Construct a new DestroyItemEvent object
  //! @param item_entity The Cmp::WorldItem entity to remove.
  explicit DestroyItemEvent( entt::entity item_entity )
      : m_item_entity( item_entity )
  {
  }

  //! @brief The Cmp::WorldItem entity to remove.
  entt::entity m_item_entity;
};

} // namespace Game::Events

#endif // SRC_EVENTS_DESTROYITEMEVENT_HPP__
