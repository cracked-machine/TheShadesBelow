#ifndef SRC_COMPONENTS_PERSISTENT_INVENTORYSTACKSIZE_HPP__
#define SRC_COMPONENTS_PERSISTENT_INVENTORYSTACKSIZE_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Maximum number of a stackable item (items.json "stackable") held in one inventory slot, see
//! Cmp::Inventory::StackCount.
class InventoryStackSize : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial stack size and optional min/max bounds.
  //! @param value initial stack size
  //! @param min_value minimum allowed stack size
  //! @param max_value maximum allowed stack size
  InventoryStackSize( int value = 1, int min_value = 1, int max_value = 1 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "InventoryStackSize"
  [[nodiscard]] std::string class_name() const override { return "InventoryStackSize"; }
  //! @brief Get the human-readable detail/description text (unset; see persist.json).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_INVENTORYSTACKSIZE_HPP__
