#ifndef SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
#define SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__

#include <Components/Inventory/WorldItem.hpp>
#include <Systems/Stores/ItemStore.hpp>

namespace Game::Cmp
{

//! @brief The player's grimoire: the set of spell entries (keyed by item id) that have been
//! discovered/enabled, and whether the grimoire overlay itself is currently shown.
class Grimoire
{
public:
  //! @brief Construct a new Grimoire object, pre-populating all known plant spell entries as disabled.
  Grimoire()
  {
    contents.emplace( "item.plant1", false );
    contents.emplace( "item.plant2", false );
    contents.emplace( "item.plant3", false );
    contents.emplace( "item.plant4", false );
    contents.emplace( "item.plant5", false );
    contents.emplace( "item.plant6", false );
  }

  //! @brief Item identifier key for a grimoire entry, e.g. "item.plant1".
  using ItemKey = Sys::ItemKey;
  //! @brief Whether a grimoire entry has been discovered/enabled.
  using Enabled = bool;
  //! @brief The grimoire's spell entries and whether each has been discovered/enabled.
  std::map<ItemKey, Enabled> contents;
  //! @brief Whether the grimoire overlay is currently enabled/shown.
  bool is_enabled{ false };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
