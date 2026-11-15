#ifndef SRC_COMPONENTS_PERSISTENT_INVENTORYSLOTCOUNT_HPP__
#define SRC_COMPONENTS_PERSISTENT_INVENTORYSLOTCOUNT_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Number of slots in the player inventory (1 to 5). Every slot always exists as an entity, empty or not.
//! @note Read via Utils::Player::get_inventory_slot_count(), which clamps the value to [1, Cmp::kMaxInventorySlots].
class InventorySlotCount : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial slot count and optional min/max bounds.
  //! @param value initial slot count
  //! @param min_value minimum allowed slot count
  //! @param max_value maximum allowed slot count
  InventorySlotCount( int value = 1, int min_value = 1, int max_value = 5 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "InventorySlotCount"
  [[nodiscard]] std::string class_name() const override { return "InventorySlotCount"; }
  //! @brief Get the human-readable detail/description text (unset; see persist.json).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_INVENTORYSLOTCOUNT_HPP__
