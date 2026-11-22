#include <Components/Inventory/Grimoire.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Npc/Npc.hpp>
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
#include <string_view>

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
  for ( const Sys::ItemKey &item_key : item_store.get_pattern( "^item\\.plant\\." ) )
  {
    const auto &item = item_store.get( item_key );
    auto &page = plant_pages.emplace_back( PlantPage{
        .plant_subpage = { .item_key = item_key, .sprite_key = "sprite.plant.unknown", .action_map = {}, .discovered_actions = {} },
        .forage_subpage = { .item_key = item.get_forage_output(), .sprite_key = "sprite.item.unknown", .action_map = {}, .discovered_actions = {} } } );
    for ( SubPage *sub_page : { &page.plant_subpage, &page.forage_subpage } )
    {
      prefill_empty_actions( *sub_page );
    }
    // a plant with no forage output (e.g. grass) has no item for its forage subpage, so it is "none": an empty-key
    // subpage can never be discovered, so it would otherwise show "?" forever
    if ( page.forage_subpage.item_key.empty() ) { prefill_none_actions( page.forage_subpage ); }
  }
}

void Grimoire::reveal_all()
{
  const auto reveal_action = [&]( SubPage &sub_page, std::string_view action_key, const BaseAction &action )
  {
    if ( action.is_zero() ) return;
    sub_page.action_map.insert_or_assign( std::string( action_key ), action );
    sub_page.discovered_actions.insert( std::string( action_key ) );
  };

  for ( NpcPage &npc_page : npc_pages )
  {
    const auto &npc = Sys::NpcStore::instance().get( npc_page.npc_key );
    if ( not npc.sprite_type_list.empty() ) { npc_page.npc_subpage.sprite_key = npc.sprite_type_list.front(); }
    npc_page.npc_subpage.sprite_index = 0;
    for_each_action_type( [&]<typename ActionT>()
    {
      if ( npc.has_action<ActionT>() ) { reveal_action( npc_page.npc_subpage, ActionT::kKey, npc.at<ActionT>().action ); }
    } );
  }

  const auto &item_store = Sys::ItemStore::instance();
  for ( PlantPage &page : plant_pages )
  {
    for ( SubPage *sub_page : { &page.plant_subpage, &page.forage_subpage } )
    {
      // an empty key is a "none" subpage; a wildcard key matches no single item and is never revealed in play
      if ( sub_page->item_key.empty() or sub_page->item_key.str().ends_with( '*' ) ) continue;
      const auto &item = item_store.get( sub_page->item_key );
      sub_page->sprite_key = item.sprite_type;
      sub_page->sprite_index = item.icon_sprite_index();
      for_each_action_type( [&]<typename ActionT>()
      {
        if ( item.has_action<ActionT>() ) { reveal_action( *sub_page, ActionT::kKey, item.get_action<ActionT>() ); }
      } );
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
