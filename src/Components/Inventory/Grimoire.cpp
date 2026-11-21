#include <Components/Inventory/Grimoire.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Stats/ActionReward.hpp>
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

#include <algorithm>

namespace Game::Cmp
{

namespace
{

//! @brief Call fn.template operator()<ActionT>() for every Cmp::BaseAction subclass (see src/Components/Stats).
template <typename Fn>
void for_each_action_type( Fn &&fn )
{
  const auto call = [&]<typename... ActionsT>() { ( fn.template operator()<ActionsT>(), ... ); };
  call.template operator()<BurnAction, CarryAction, CollisionAction, ConsumeAction, DestroyAction, ForageAction, ProjectileAction, ProximityAction,
                           OfferingAction, SpawnAction>();
}

} // namespace

Grimoire::Grimoire()
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
        .drop_subpage = { .item_key = item.get_drop_output(), .sprite_key = "sprite.item.unknown", .action_map = {}, .discovered_actions = {} },
        .plant_offering_subpage =
            { .item_key = offering_subpage_key( item_key ), .sprite_key = "sprite.item.unknown", .action_map = {}, .discovered_actions = {} },
        .forage_offering_subpage = { .item_key = offering_subpage_key( item.get_forage_output() ),
                                     .sprite_key = "sprite.item.unknown",
                                     .action_map = {},
                                     .discovered_actions = {} },
        .drop_offering_subpage = { .item_key = offering_subpage_key( item.get_drop_output() ),
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

void Grimoire::prefill_empty_actions( SubPage &sub_page )
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

void Grimoire::prefill_empty_npc_actions( const Sys::NpcKey &npc_key, SubPage &sub_page )
{
  const auto &npc = Sys::NpcStore::instance().get( npc_key );
  for_each_action_type( [&]<typename ActionT>()
  {
    const auto &action = npc.at<ActionT>().action;
    if ( action.is_zero() ) { sub_page.action_map.emplace( std::string( ActionT::kKey ), action ); }
  } );
}

void Grimoire::prefill_none_actions( SubPage &sub_page )
{
  sub_page.sprite_key = "sprite.item.none";
  sub_page.sprite_index = 0;
  for_each_action_type( [&]<typename ActionT>()
  { sub_page.action_map.emplace( std::string( ActionT::kKey ), BaseAction( {}, {}, {}, {}, {}, {} ) ); } );
}

} // namespace Game::Cmp
