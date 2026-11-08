#ifndef SRC_EVENTS_TALISMANREFUSEDEVENT_HPP__
#define SRC_EVENTS_TALISMANREFUSEDEVENT_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <utility>

namespace Game::Events
{

//! @brief Notification that an altar refused a talisman because an altar is already fully activated (Cmp::Altar::Activated),
//! so its grave trail must be completed before any other altar can be activated.
//! @note Sent by Sys::AltarSystem. See Sys::TooltipSystem.
struct TalismanRefusedEvent
{
  //! @brief Construct a new TalismanRefusedEvent object
  //! @param item Item identifier of the refused talisman, e.g. "item.talisman1". See res/json/items.json.
  explicit TalismanRefusedEvent( Sys::ItemKey item )
      : m_item( std::move( item ) )
  {
  }

  //! @brief Item identifier of the refused talisman.
  Sys::ItemKey m_item;
};

} // namespace Game::Events

#endif // SRC_EVENTS_TALISMANREFUSEDEVENT_HPP__
