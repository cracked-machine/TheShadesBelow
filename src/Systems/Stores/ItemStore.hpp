#ifndef SRC_SYSTEMS_STORES_ITEMSTORE_HPP__
#define SRC_SYSTEMS_STORES_ITEMSTORE_HPP__

#include <Components/Inventory/WorldItem.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Systems/Stores/StoreSingleton.hpp>

namespace Game::Sys
{

//! @brief Singleton store of item metadata (Cmp::WorldItem, keyed by item id) loaded from res/json/items.json.
class ItemStore : public StoreSingleton<ItemStore, Cmp::WorldItem>
{
public:
  //! @brief Construct a new Item Store object
  //! @param json_file_path Path of the JSON file to load; override to load a fixture (e.g. in tests).
  explicit ItemStore( std::filesystem::path json_file_path = "res/json/items.json" );

  //! @brief Destroy the Item Store object
  ~ItemStore() {}

private:
  //! @brief Populates m_store with InventoryItem components
  void init_store();
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_STORES_ITEMSTORE_HPP__
