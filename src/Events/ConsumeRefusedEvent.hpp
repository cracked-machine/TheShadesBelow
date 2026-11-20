#ifndef SRC_EVENTS_CONSUMEREFUSEDEVENT_HPP__
#define SRC_EVENTS_CONSUMEREFUSEDEVENT_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <utility>

namespace Game::Events
{

//! @brief Notification that the player tried to consume an item that would be wasted, e.g. drinking a blood vial at
//! full health.
//! @note Sent by Sys::InventorySystem. See Sys::TooltipOverlaySystem.
struct ConsumeRefusedEvent
{
  //! @brief Construct a new ConsumeRefusedEvent object
  //! @param item Item identifier of the refused item, e.g. "item.grave.blood.small". See res/json/items.json.
  explicit ConsumeRefusedEvent( Sys::ItemKey item )
      : m_item( std::move( item ) )
  {
  }

  //! @brief Item identifier of the refused item.
  Sys::ItemKey m_item;
};

} // namespace Game::Events

#endif // SRC_EVENTS_CONSUMEREFUSEDEVENT_HPP__
