#ifndef SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
#define SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <map>
#include <set>
#include <string>
#include <vector>

namespace Game::Cmp
{

//! @brief The player's grimoire: one page per NPC (first) and per plant (after the NPCs), with the actions that have
//! been discovered on each, and whether the grimoire overlay itself is currently shown.
class Grimoire
{
public:
  using ActionMap = std::map<std::string, BaseAction>;
  struct SubPage
  {
    //! @brief The item this subpage describes. Empty for an NPC subpage, see NpcPage::npc_key.
    Sys::ItemKey item_key = "";
    Sys::SpriteKey sprite_key = "";
    //! @brief The frame of sprite_key to draw, e.g. the item's sprite.index from items.json.
    std::size_t sprite_index = 0;
    ActionMap action_map;
    //! @brief Keys of the actions revealed by discovery. Excludes the prefilled "none" entries, which are in action_map
    //! from the start.
    std::set<std::string> discovered_actions;
  };

  //! @brief A plant page: the plant, its forage and chop (drop) outputs, and the rewards for offering each of them.
  struct PlantPage
  {
    SubPage plant_subpage;
    SubPage forage_subpage;
    SubPage drop_subpage;
    //! @brief Reward for offering the plant item.
    SubPage plant_offering_subpage;
    //! @brief Reward for offering the forage item.
    SubPage forage_offering_subpage;
    //! @brief Reward for offering the drop item.
    SubPage drop_offering_subpage;
  };

  //! @brief An NPC page: the NPC's actions on the player stats, e.g. spawn, proximity or collision.
  struct NpcPage
  {
    //! @brief The NPC this page describes, see res/json/npc.json.
    Sys::NpcKey npc_key = "";
    SubPage npc_subpage;
  };

  //! @brief The grimoire's tab groups. Each tab pages through its own page list.
  enum class Tab { NPCS, PLANTS };

  //! @brief The NPC pages, one per NPC in Sys::NpcStore order.
  std::vector<NpcPage> npc_pages;

  //! @brief The plant pages, one per plant item in Sys::ItemStore plant order.
  std::vector<PlantPage> plant_pages;

  //! @brief Construct a new Grimoire object, populating one page per NPC in Sys::NpcStore, then one page per plant
  //! item in Sys::ItemStore.
  Grimoire();

  //! @brief Whether the grimoire overlay is currently enabled/shown.
  bool is_enabled{ false };

private:
  //! @brief Add each of the subpage item's actions that has no effect (see BaseAction::is_zero), or is absent from
  //! items.json, to the subpage, so it shows "none" before it is discovered.
  static void prefill_empty_actions( SubPage &sub_page );

  //! @brief Add each of the NPC's actions that has no effect (see BaseAction::is_zero), or is absent from npc.json, to
  //! its subpage, so it shows "none" before it is discovered.
  static void prefill_empty_npc_actions( const Sys::NpcKey &npc_key, SubPage &sub_page );

  //! @brief Show the subpage as "sprite.item.none" (icon and "None" name) with an empty action for every action key,
  //! so all of its actions show "none".
  static void prefill_none_actions( SubPage &sub_page );
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
