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

  //! @brief Get the item keys a reward item refers to (see Cmp::ActionReward::item).
  //! @param key An item key, or an item key prefix ending with '*', e.g. "item.jewelry*".
  //! @return All item keys starting with the prefix, else the key itself if it exists, else empty.
  [[nodiscard]] std::vector<key_type> get_item_matches( const key_type &key ) const;

  //! @brief Resolve a reward item to a concrete item key, picking a random match if it ends with '*'.
  //! @param key An item key, or an item key prefix ending with '*'.
  //! @return The item key.
  //! @throws std::runtime_error if no item matches.
  [[nodiscard]] key_type pick_item( const key_type &key ) const;

private:
  //! @brief Populates m_store with InventoryItem components
  void init_store();
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_STORES_ITEMSTORE_HPP__
