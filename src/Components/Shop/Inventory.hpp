#ifndef SRC_COMPONENTS_SHOP_INVENTORY_HPP__
#define SRC_COMPONENTS_SHOP_INVENTORY_HPP__

#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Constants.hpp>

namespace Game::Cmp::Shop
{

//! @brief Per-shop-entity inventory of purchasable items and their prices.
class Inventory
{
public:
  //! @brief Generation parameters used when populating a shop's inventory.
  struct Config
  {
    //! @brief Maximum number of item slots the shop inventory can hold.
    int max_items;
    //! @brief Minimum price an item may be listed for.
    int min_price;
    //! @brief Maximum price an item may be listed for.
    int max_price;
  };

  //! @brief Construct an empty Inventory with default-initialized slots and config.
  Inventory() = default;

  //! @brief Item slots currently in the shop: (item type, price) pairs.
  std::vector<std::pair<Sys::ItemKey, int32_t>> m_slots;

  //! @brief Generation parameters used to populate m_slots.
  Config m_config;

  //! @brief Whether the shop's inventory UI is currently enabled/visible.
  bool is_enabled{ false };
};

} // namespace Game::Cmp::Shop

#endif // SRC_COMPONENTS_SHOP_INVENTORY_HPP__
