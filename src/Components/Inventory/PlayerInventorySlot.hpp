#ifndef SRC_COMPONENTS_INVENTORY_PLAYERINVENTORYSLOT_HPP__
#define SRC_COMPONENTS_INVENTORY_PLAYERINVENTORYSLOT_HPP__

#include <Components/Inventory/WorldItem.hpp>

#include <cstdint>
#include <optional>

namespace Game::Cmp
{

//! @brief Maximum number of player inventory slots (see Cmp::Persist::InventorySlotCount).
inline constexpr uint8_t kMaxInventorySlots = 5;

//! @brief Represents a single slot in the player inventory.
//! @details Every slot always exists as its own entity, one per index `0..count-1`, whether it holds an item or not
//! (see Factory::Player::ensure_inventory_slots). A slot's position (HUD column, number key) is its m_slot_idx only:
//! emptying or filling a slot changes its contents in place and never shifts other slots.
//! Item state (wear, sting, UUID, sprite frame, ...) lives as separate components on the slot entity; an empty slot has
//! none of them. Factory::Player::clear_inventory_slot lists them - add any new per-item component there.
struct PlayerInventorySlot
{
  //! @brief Fixed position of this slot in the inventory.
  uint8_t m_slot_idx{ 0 };

  //! @brief The item held in this inventory slot, or std::nullopt if the slot is empty.
  std::optional<WorldItem> m_item;

  //! @brief Whether the slot holds no item.
  [[nodiscard]] bool empty() const { return not m_item.has_value(); }
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_PLAYERINVENTORYSLOT_HPP__
