#ifndef SRC_COMPONENTS_PERSISTENT_INVENTORYSLOTROLLOVER_HPP__
#define SRC_COMPONENTS_PERSISTENT_INVENTORYSLOTROLLOVER_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Whether next/prev inventory slot selection wraps around at the first/last slot, or stops there.
//! See Sys::InventorySystem::on_select_inventory_slot_event().
class InventorySlotRollover : public BasePersistent<bool>
{
public:
  //! @brief Construct the inventory slot rollover setting.
  //! @param value true to wrap around, false to stop at the first/last slot.
  InventorySlotRollover( bool value = true )
      : BasePersistent<bool>( value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "InventorySlotRollover"; }
  //! @brief Returns the display description for this setting (unset; see persist.json).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_INVENTORYSLOTROLLOVER_HPP__
