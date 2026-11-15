#ifndef SRC_EVENTS_SELECTINVENTORYSLOTEVENT_HPP__
#define SRC_EVENTS_SELECTINVENTORYSLOTEVENT_HPP__

#include <cstdint>

namespace Game::Events
{

//! @brief Request to change the player's selected inventory slot. Handled by Sys::InventorySystem.
struct SelectInventorySlotEvent
{
  //! @brief How the new slot is chosen.
  enum class Mode {
    //! @brief Select the slot at m_idx (ignored if out of range).
    INDEX,
    //! @brief Select the next slot, wrapping around.
    NEXT,
    //! @brief Select the previous slot, wrapping around.
    PREV
  };

  //! @brief Construct a new SelectInventorySlotEvent
  //! @param mode How the new slot is chosen.
  //! @param idx The slot index, only used for Mode::INDEX.
  explicit SelectInventorySlotEvent( Mode mode, uint8_t idx = 0 )
      : m_mode( mode ),
        m_idx( idx )
  {
  }

  //! @brief How the new slot is chosen.
  Mode m_mode;
  //! @brief The slot index for Mode::INDEX.
  uint8_t m_idx;
};

} // namespace Game::Events

#endif // SRC_EVENTS_SELECTINVENTORYSLOTEVENT_HPP__
