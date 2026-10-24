#ifndef SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
#define SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__

#include <Components/Inventory/WorldItem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/StoreKey.hpp>

namespace Game::Cmp
{

//! @brief The player's grimoire: the set of spell entries (keyed by item id) that have been
//! discovered/enabled, and whether the grimoire overlay itself is currently shown.
class Grimoire
{
public:
  struct Page
  {
    Sys::SpriteKey sprite_key;
  };

  //! @brief Construct a new Grimoire object, pre-populating all known plant spell entries as disabled.
  Grimoire()
  {
    contents.emplace( "item.plant1", false );
    contents.emplace( "item.plant2", false );
    contents.emplace( "item.plant3", false );
    contents.emplace( "item.plant4", false );
    contents.emplace( "item.plant5", false );
    contents.emplace( "item.plant6", false );

    pages.emplace( "item.plant.rowan", Page{ .sprite_key = "sprite.item.plant.rowan" } );
    pages.emplace( "item.plant.elder", Page{ .sprite_key = "sprite.item.plant.elder" } );
    pages.emplace( "item.plant.yew", Page{ .sprite_key = "sprite.item.plant.yew" } );
    pages.emplace( "item.plant.hawthorn", Page{ .sprite_key = "sprite.item.plant.hawthorn" } );
    pages.emplace( "item.plant.foxglove", Page{ .sprite_key = "sprite.item.plant.foxglove" } );
    pages.emplace( "item.plant.nightshade", Page{ .sprite_key = "sprite.item.plant.nightshade" } );
    pages.emplace( "item.plant.hogweed", Page{ .sprite_key = "sprite.item.plant.hogweed" } );
    pages.emplace( "item.plant.stjohnswort", Page{ .sprite_key = "sprite.item.plant.stjohnswort" } );
    pages.emplace( "item.plant.nettle", Page{ .sprite_key = "sprite.item.plant.nettle" } );
    pages.emplace( "item.plant.burdock", Page{ .sprite_key = "sprite.item.plant.burdock" } );
    pages.emplace( "item.plant.redcampion", Page{ .sprite_key = "sprite.item.plant.redcampion" } );
    pages.emplace( "item.plant.henbane", Page{ .sprite_key = "sprite.item.plant.henbane" } );
  }

  //! @brief Item identifier key for a grimoire entry, e.g. "item.plant1".
  using ItemKey = Sys::ItemKey;
  //! @brief Whether a grimoire entry has been discovered/enabled.
  using Enabled = bool;
  //! @brief The grimoire's spell entries and whether each has been discovered/enabled.
  std::map<ItemKey, Enabled> contents;
  //! @brief Whether the grimoire overlay is currently enabled/shown.
  bool is_enabled{ false };

  std::unordered_map<ItemKey, Page> pages;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
