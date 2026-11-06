#ifndef SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
#define SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__

#include <Components/Inventory/WorldItem.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/CollisionAction.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ForageAction.hpp>
#include <Components/Stats/ProjectileAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/OfferingAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Game::Cmp
{

//! @brief The player's grimoire: the set of spell entries (keyed by item id) that have been
//! discovered/enabled, and whether the grimoire overlay itself is currently shown.
class Grimoire
{
public:
  using ActionMap = std::map<std::string, BaseAction>;
  struct SubPage
  {
    Sys::ItemKey item_key = "";
    Sys::SpriteKey sprite_key = "";
    ActionMap action_map;
    //! @brief Keys of the actions revealed by discovery. Excludes the prefilled "none" entries, which are in action_map
    //! from the start.
    std::set<std::string> discovered_actions;
  };

  struct Page
  {
    SubPage plant_subpage;
    SubPage forage_subpage;
    SubPage drop_subpage;
    //! @brief Reward for offering the forage item.
    SubPage forage_offering_subpage;
    //! @brief Reward for offering the drop item.
    SubPage drop_offering_subpage;
  };

  //! @brief Page list type, in Sys::ItemStore plant order.
  using Pages = std::vector<Page>;

  //! @brief The grimoire's pages, one per plant item.
  Pages pages;

  //! @brief Construct a new Grimoire object, populating one page per plant item in Sys::ItemStore.
  Grimoire()
  {
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
      pages.push_back( Page{ .plant_subpage = { .item_key = item_key, .sprite_key = "sprite.plant.unknown", .action_map = {} },
                             .forage_subpage = { .item_key = item.get_forage_output(), .sprite_key = "sprite.item.unknown", .action_map = {} },
                             .drop_subpage = { .item_key = item.get_axe_output(), .sprite_key = "sprite.item.unknown", .action_map = {} },
                             .forage_offering_subpage = { .item_key = offering_subpage_key( item.get_forage_output() ),
                                                           .sprite_key = "sprite.item.unknown",
                                                           .action_map = {} },
                             .drop_offering_subpage = {
                                 .item_key = offering_subpage_key( item.get_axe_output() ), .sprite_key = "sprite.item.unknown", .action_map = {} } } );
      auto &page = pages.back();
      for ( SubPage *sub_page :
            { &page.plant_subpage, &page.forage_subpage, &page.drop_subpage, &page.forage_offering_subpage, &page.drop_offering_subpage } )
      {
        prefill_empty_actions( *sub_page );
      }
      // an item whose offering does nothing, or rewards a wildcard item, has no offering reward subpage, so every
      // action of its offering subpage is "none"
      for ( const auto &[item_subpage, offering_subpage] :
            { std::pair{ &page.forage_subpage, &page.forage_offering_subpage }, std::pair{ &page.drop_subpage, &page.drop_offering_subpage } } )
      {
        if ( item_subpage->action_map.contains( std::string( OfferingAction::kKey ) ) or is_wildcard_reward( item_subpage->item_key ) )
        {
          prefill_none_actions( *offering_subpage );
        }
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

  //! @brief Add each of the subpage item's actions that has no effect (see BaseAction::is_zero) to the subpage, so it
  //! shows "none" before it is discovered.
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
        return item.has_action<ActionT>() and item.get_action<ActionT>().is_zero();
      } );
      if ( all_empty ) { sub_page.action_map.emplace( std::string( ActionT::kKey ), item_store.get( matches.front() ).get_action<ActionT>() ); }
    } );
  }

  //! @brief Show the subpage as "sprite.item.none" (icon and "None" name) with an empty action for every action key,
  //! so all of its actions show "none".
  static void prefill_none_actions( SubPage &sub_page )
  {
    sub_page.sprite_key = "sprite.item.none";
    for_each_action_type( [&]<typename ActionT>()
    { sub_page.action_map.emplace( std::string( ActionT::kKey ), BaseAction( {}, {}, {}, {}, {}, {} ) ); } );
  }
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
