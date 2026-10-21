#ifndef SRC_EVENTS_ITEMPICKEDUPEVENT_HPP__
#define SRC_EVENTS_ITEMPICKEDUPEVENT_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <utility>

namespace Game::Events
{

//! @brief Notification that a world item has been picked up into the player inventory.
//! @note Sent by Sys::InventorySystem after the pickup has completed, for every pickup path
//! (swap inventory action and Events::PickupWorldItemEvent). The world item entity no longer exists.
struct ItemPickedUpEvent
{
  //! @brief Construct a new ItemPickedUpEvent object
  //! @param item Item identifier, e.g. "item.witchesbroom". See res/json/items.json.
  explicit ItemPickedUpEvent( Sys::ItemKey item )
      : m_item( std::move( item ) )
  {
  }

  //! @brief Item identifier of the item that was picked up.
  Sys::ItemKey m_item;
};

} // namespace Game::Events

#endif // SRC_EVENTS_ITEMPICKEDUPEVENT_HPP__
