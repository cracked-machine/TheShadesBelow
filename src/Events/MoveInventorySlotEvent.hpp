#ifndef SRC_EVENTS_MOVEINVENTORYSLOTEVENT_HPP__
#define SRC_EVENTS_MOVEINVENTORYSLOTEVENT_HPP__

#include <cstdint>

namespace Game::Events
{

//! @brief Request to move the item in one inventory slot to another (e.g. by dragging it on the HUD). An empty
//! destination slot takes the item; an occupied one swaps items with the source. Handled by Sys::InventorySystem.
struct MoveInventorySlotEvent
{
  //! @brief Construct a new MoveInventorySlotEvent
  //! @param from The slot index the item is moved from.
  //! @param to The slot index the item is moved to.
  MoveInventorySlotEvent( uint8_t from, uint8_t to )
      : m_from( from ),
        m_to( to )
  {
  }

  //! @brief The slot index the item is moved from.
  uint8_t m_from;
  //! @brief The slot index the item is moved to.
  uint8_t m_to;
};

} // namespace Game::Events

#endif // SRC_EVENTS_MOVEINVENTORYSLOTEVENT_HPP__
