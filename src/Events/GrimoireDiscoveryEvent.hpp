#ifndef SRC_EVENTS_GRIMOIREDISCOVERYEVENT_HPP__
#define SRC_EVENTS_GRIMOIREDISCOVERYEVENT_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <utility>

namespace Game::Events
{

//! @brief Notification that the grimoire revealed new information about an item: its page entry or a newly
//! discovered action.
//! @note Sent by Sys::GrimoireSystem only when the grimoire actually changed, unlike Events::GrimoireUpdateEvent
//! which is sent on every application of an action. See Sys::TooltipSystem.
struct GrimoireDiscoveryEvent
{
  //! @brief Construct a new GrimoireDiscoveryEvent object
  //! @param item Item identifier, e.g. "item.plant.nightshade". See res/json/items.json.
  explicit GrimoireDiscoveryEvent( Sys::ItemKey item )
      : m_item( std::move( item ) )
  {
  }

  //! @brief Item identifier of the item whose grimoire entry changed.
  Sys::ItemKey m_item;
};

} // namespace Game::Events

#endif // SRC_EVENTS_GRIMOIREDISCOVERYEVENT_HPP__
