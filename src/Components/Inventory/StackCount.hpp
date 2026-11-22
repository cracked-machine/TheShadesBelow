#ifndef SRC_COMPONENTS_INVENTORY_STACKCOUNT_HPP__
#define SRC_COMPONENTS_INVENTORY_STACKCOUNT_HPP__

namespace Game::Cmp::Inventory
{

//! @brief How many of a stackable item (items.json "stackable", see Cmp::WorldItem::is_stackable()) an inventory slot or a
//! world item holds. An entity without one holds a single item, see Utils::Player::get_stack_count().
//! @details A stack is capped at Cmp::Persist::InventoryStackSize. Dropping a slot drops its whole stack as one world item,
//! which keeps this count, so picking it back up restores the stack. A stack shares one Cmp::Inventory::WearLevel, the
//! count-weighted average of the items merged into it (see Sys::InventorySystem::merge_into_stack()), so it rots as one.
class StackCount
{
public:
  //! @brief Construct a new Stack Count object.
  //! @param count The number of items in the stack.
  explicit StackCount( int count )
      : m_count( count )
  {
  }

  //! @brief The number of items in the stack.
  int m_count;
};

} // namespace Game::Cmp::Inventory

#endif // SRC_COMPONENTS_INVENTORY_STACKCOUNT_HPP__
