#ifndef SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
#define SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__

#include <Components/Inventory/WorldItem.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/CollisionAction.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ForageAction.hpp>
#include <Components/Stats/OfferingAction.hpp>
#include <Components/Stats/ProjectileAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <algorithm>
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
  Grimoire()
  {
    for ( const Sys::NpcKey &npc_key : Sys::NpcStore::instance().get_pattern( "^npc\\." ) )
    {
      NpcPage npc_page{ .npc_key = npc_key,
                        .npc_subpage = { .item_key = "", .sprite_key = "sprite.npc.unknown", .action_map = {}, .discovered_actions = {} } };
      prefill_empty_npc_actions( npc_key, npc_page.npc_subpage );
      npc_pages.push_back( std::move( npc_page ) );
    }

    const auto &item_store = Sys::ItemStore::instance();
    const auto offering_reward = [&]( const Sys::ItemKey &key )
    {
      // a wildcard output has no single item to look up
      if ( key.empty() or key.str().ends_with( '*' ) ) return Sys::ItemKey( "" );
      // plants are offered at the graveyard altars
      const auto reward = item_store.get( key ).get_offering_reward( Cmp::Altar::Kind::GRAVEYARD );
      return reward ? reward->item : Sys::ItemKey( "" );
    };
    // a wildcard reward (e.g. Nightshade's "item.jewelry*") is not a plant item, so it is never added to the grimoire
    const auto is_wildcard_reward = [&]( const Sys::ItemKey &key ) { return offering_reward( key ).str().ends_with( '*' ); };
    const auto offering_subpage_key = [&]( const Sys::ItemKey &key )
    { return is_wildcard_reward( key ) ? Sys::ItemKey( "" ) : offering_reward( key ); };
    for ( const Sys::ItemKey &item_key : item_store.get_pattern( "^item\\.plant\\." ) )
    {
      const auto &item = item_store.get( item_key );
      auto &page = plant_pages.emplace_back( PlantPage{
          .plant_subpage = { .item_key = item_key, .sprite_key = "sprite.plant.unknown", .action_map = {}, .discovered_actions = {} },
          .forage_subpage = { .item_key = item.get_forage_output(), .sprite_key = "sprite.item.unknown", .action_map = {}, .discovered_actions = {} },
          .drop_subpage = { .item_key = item.get_axe_output(), .sprite_key = "sprite.item.unknown", .action_map = {}, .discovered_actions = {} },
          .plant_offering_subpage = { .item_key = offering_subpage_key( item_key ),
                                      .sprite_key = "sprite.item.unknown",
                                      .action_map = {},
                                      .discovered_actions = {} },
          .forage_offering_subpage = { .item_key = offering_subpage_key( item.get_forage_output() ),
                                       .sprite_key = "sprite.item.unknown",
                                       .action_map = {},
                                       .discovered_actions = {} },
          .drop_offering_subpage = { .item_key = offering_subpage_key( item.get_axe_output() ),
                                     .sprite_key = "sprite.item.unknown",
                                     .action_map = {},
                                     .discovered_actions = {} } } );
      for ( SubPage *sub_page : { &page.plant_subpage, &page.forage_subpage, &page.drop_subpage, &page.plant_offering_subpage,
                                  &page.forage_offering_subpage, &page.drop_offering_subpage } )
      {
        prefill_empty_actions( *sub_page );
      }
      // an item whose offering rewards no item (it does nothing, only changes stats, triggers an altar effect such as
      // Nightshade's extra life, or rewards a wildcard item) has no offering reward subpage, so every action of its
      // offering subpage is "none". The offering's own effects are shown on the item's offering_action instead.
      for ( SubPage *offering_subpage : { &page.plant_offering_subpage, &page.forage_offering_subpage, &page.drop_offering_subpage } )
      {
        if ( offering_subpage->item_key.empty() ) { prefill_none_actions( *offering_subpage ); }
      }
    }
  }

  //! @brief Whether the grimoire overlay is currently enabled/shown.
  bool is_enabled{ false };

private:
  //! @brief Call fn.template operator()<ActionT>() for every Cmp::BaseAction subclass (see src/Components/Stats).
  template <typename Fn>
  static void for_each_action_type( Fn &&fn )
  {
    const auto call = [&]<typename... ActionsT>() { ( fn.template operator()<ActionsT>(), ... ); };
    call.template operator()<BurnAction, CarryAction, CollisionAction, ConsumeAction, DestroyAction, ForageAction, ProjectileAction, ProximityAction,
                             OfferingAction, SpawnAction>();
  }

  //! @brief Add each of the subpage item's actions that has no effect (see BaseAction::is_zero), or is absent from
  //! items.json, to the subpage, so it shows "none" before it is discovered.
  static void prefill_empty_actions( SubPage &sub_page )
  {
    if ( sub_page.item_key.empty() ) return;
    const auto &item_store = Sys::ItemStore::instance();
    const auto matches = item_store.get_item_matches( sub_page.item_key );
    if ( matches.empty() ) return;
    for_each_action_type( [&]<typename ActionT>()
    {
      const bool all_empty = std::ranges::all_of( matches, [&]( const Sys::ItemKey &key )
      {
        const auto &item = item_store.get( key );
        return item.get_action<ActionT>().is_zero();
      } );
      if ( all_empty ) { sub_page.action_map.emplace( std::string( ActionT::kKey ), item_store.get( matches.front() ).get_action<ActionT>() ); }
    } );
  }

  //! @brief Add each of the NPC's actions that has no effect (see BaseAction::is_zero), or is absent from npc.json, to
  //! its subpage, so it shows "none" before it is discovered.
  static void prefill_empty_npc_actions( const Sys::NpcKey &npc_key, SubPage &sub_page )
  {
    const auto &npc = Sys::NpcStore::instance().get( npc_key );
    for_each_action_type( [&]<typename ActionT>()
    {
      const auto &action = npc.at<ActionT>().action;
      if ( action.is_zero() ) { sub_page.action_map.emplace( std::string( ActionT::kKey ), action ); }
    } );
  }

  //! @brief Show the subpage as "sprite.item.none" (icon and "None" name) with an empty action for every action key,
  //! so all of its actions show "none".
  static void prefill_none_actions( SubPage &sub_page )
  {
    sub_page.sprite_key = "sprite.item.none";
    sub_page.sprite_index = 0;
    for_each_action_type( [&]<typename ActionT>()
    { sub_page.action_map.emplace( std::string( ActionT::kKey ), BaseAction( {}, {}, {}, {}, {}, {}, {} ) ); } );
  }
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
