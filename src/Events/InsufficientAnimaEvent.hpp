#ifndef SRC_EVENTS_INSUFFICIENTANIMAEVENT_HPP__
#define SRC_EVENTS_INSUFFICIENTANIMAEVENT_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <utility>

namespace Game::Events
{

//! @brief Notification that an altar refused an item because the player does not have enough anima to pay for it.
//! @note Sent by Sys::AltarSystem when a talisman is offered without enough anima. See Sys::TooltipSystem.
struct InsufficientAnimaEvent
{
  //! @brief Construct a new InsufficientAnimaEvent object
  //! @param item Item identifier of the refused item, e.g. "item.talisman1". See res/json/items.json.
  explicit InsufficientAnimaEvent( Sys::ItemKey item )
      : m_item( std::move( item ) )
  {
  }

  //! @brief Item identifier of the refused item.
  Sys::ItemKey m_item;
};

} // namespace Game::Events

#endif // SRC_EVENTS_INSUFFICIENTANIMAEVENT_HPP__
