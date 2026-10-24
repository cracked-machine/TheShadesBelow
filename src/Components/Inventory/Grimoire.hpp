#ifndef SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
#define SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__

#include <Components/Inventory/WorldItem.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <map>

namespace Game::Cmp
{

//! @brief The player's grimoire: the set of spell entries (keyed by item id) that have been
//! discovered/enabled, and whether the grimoire overlay itself is currently shown.
class Grimoire
{
public:
  using ActionMap = std::map<std::string, BaseAction>;
  struct Page
  {
    Sys::SpriteKey sprite_key;
    ActionMap action_map;
  };

  //! @brief Page map type, ordered by item key so pages have a stable order.
  using PageMap = std::map<Sys::ItemKey, Page>;

  //! @brief Construct a new Grimoire object, populating one page per plant item in Sys::ItemStore.
  Grimoire()
  {
    const auto &item_store = Sys::ItemStore::instance();
    for ( const auto &item_key : item_store.get_pattern( "^item\\.plant\\." ) )
    {
      pages.emplace( item_key, Page{ .sprite_key = "", .action_map = {} } );
    }
  }

  //! @brief Whether the grimoire overlay is currently enabled/shown.
  bool is_enabled{ false };

  //! @brief The grimoire's pages, keyed by item.
  PageMap pages;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
