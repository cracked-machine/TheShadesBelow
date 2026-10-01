#ifndef SRC_SYSTEMS_STORES_ITEMSTORE_HPP__
#define SRC_SYSTEMS_STORES_ITEMSTORE_HPP__

#include <Components/Inventory/WorldItem.hpp>
#include <Sprites/SpriteMetaType.hpp>
#include <Systems/Stores/StoreSingleton.hpp>

namespace Game::Sys
{

//! @brief Singleton store of item metadata (Cmp::WorldItem, keyed by item id) loaded from res/json/items.json.
class ItemStore : public StoreSingleton<ItemStore, Cmp::WorldItem>
{
public:
  //! @brief Construct a new Item Store object
  ItemStore();

  //! @brief Destroy the Item Store object
  ~ItemStore() {}

private:
  //! @brief Populates m_store with InventoryItem components
  void init_store();
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_STORES_ITEMSTORE_HPP__
