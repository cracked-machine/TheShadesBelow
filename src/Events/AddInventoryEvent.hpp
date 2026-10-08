#ifndef SRC_EVENTS_ADDINVENTORYEVENT_HPP__
#define SRC_EVENTS_ADDINVENTORYEVENT_HPP__

#include <Systems/Stores/StoreKey.hpp>
#include <utility>

namespace Game::Events
{

//! @brief Requests that a new item be created directly in a player inventory slot.
struct AddInventoryEvent
{

  //! @brief Construct a new AddInventoryEvent object
  //! @param item Item identifier, e.g. "item.pickaxe". See res/json/items.json.
  explicit AddInventoryEvent( Sys::ItemKey item )
      : m_item( std::move( item ) )
  {
  }

  //! @brief Item identifier of the item to create.
  Sys::ItemKey m_item;
};

} // namespace Game::Events

#endif // SRC_EVENTS_ADDINVENTORYEVENT_HPP__
