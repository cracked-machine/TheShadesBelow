#ifndef SRC_EVENTS_DROPINVENTORYSLOTEVENT_HPP__
#define SRC_EVENTS_DROPINVENTORYSLOTEVENT_HPP__

#include <cstdint>

namespace Game::Events
{

//! @brief Request to drop the whole stack in an inventory slot into the world (e.g. by dragging it off the HUD), on the
//! free cell beside the player nearest to them. A plant is replanted at the player's feet instead, as a normal drop
//! does. Nothing is dropped if there is nowhere to put it. Handled by Sys::InventorySystem.
struct DropInventorySlotEvent
{
  //! @brief Construct a new DropInventorySlotEvent
  //! @param slot The slot index whose item is dropped.
  explicit DropInventorySlotEvent( uint8_t slot )
      : m_slot( slot )
  {
  }

  //! @brief The slot index whose item is dropped.
  uint8_t m_slot;
};

} // namespace Game::Events

#endif // SRC_EVENTS_DROPINVENTORYSLOTEVENT_HPP__
