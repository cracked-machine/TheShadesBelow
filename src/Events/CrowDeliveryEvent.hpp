#ifndef SRC_EVENTS_CROWDELIVERYEVENT_HPP__
#define SRC_EVENTS_CROWDELIVERYEVENT_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <utility>

namespace Game::Events
{

//! @brief Send a crow to fetch the least worn world item of type m_item and drop it beside the player.
//! @note Consumed by Sys::NpcSystem. The item is moved, not copied. No op if there is no such item in the world.
//! Enqueue rather than trigger when the caller may still be iterating a Cmp::Position view.
struct CrowDeliveryEvent
{
  //! @brief Construct a new CrowDeliveryEvent object
  //! @param item Item identifier, e.g. "item.pickaxe". See res/json/items.json.
  explicit CrowDeliveryEvent( Sys::ItemKey item )
      : m_item( std::move( item ) )
  {
  }

  //! @brief Item identifier of the world item to deliver.
  Sys::ItemKey m_item;
};

} // namespace Game::Events

#endif // SRC_EVENTS_CROWDELIVERYEVENT_HPP__
