#ifndef SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
#define SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__

#include <Components/Inventory/WorldItem.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <map>
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
  };

  struct Page
  {
    SubPage plant_subpage;
    SubPage forage_subpage;
    SubPage drop_subpage;
    //! @brief Reward for sacrificing the forage item.
    SubPage forage_special_subpage;
    //! @brief Reward for sacrificing the drop item.
    SubPage drop_special_subpage;
  };

  //! @brief Page list type, in Sys::ItemStore plant order.
  using Pages = std::vector<Page>;

  //! @brief The grimoire's pages, one per plant item.
  Pages pages;

  //! @brief Construct a new Grimoire object, populating one page per plant item in Sys::ItemStore.
  Grimoire()
  {
    const auto &item_store = Sys::ItemStore::instance();
    const auto sacrifice_reward = [&]( const Sys::ItemKey &key )
    {
      // a wildcard output has no single item to look up
      if ( key.empty() or key.str().ends_with( '*' ) ) return Sys::ItemKey( "" );
      // plants are offered at the graveyard altars
      const auto reward = item_store.get( key ).get_sacrifice_reward( Cmp::Altar::Kind::GRAVEYARD );
      return reward ? reward->item : Sys::ItemKey( "" );
    };
    for ( const Sys::ItemKey &item_key : item_store.get_pattern( "^item\\.plant\\." ) )
    {
      const auto &item = item_store.get( item_key );
      pages.push_back( Page{ .plant_subpage = { .item_key = item_key, .sprite_key = "sprite.plant.unknown", .action_map = {} },
                             .forage_subpage = { .item_key = item.get_forage_output(), .sprite_key = "sprite.item.unknown", .action_map = {} },
                             .drop_subpage = { .item_key = item.get_axe_output(), .sprite_key = "sprite.item.unknown", .action_map = {} },
                             .forage_special_subpage = { .item_key = sacrifice_reward( item.get_forage_output() ),
                                                         .sprite_key = "sprite.item.unknown",
                                                         .action_map = {} },
                             .drop_special_subpage = {
                                 .item_key = sacrifice_reward( item.get_axe_output() ), .sprite_key = "sprite.item.unknown", .action_map = {} } } );
    }
  }

  //! @brief Whether the grimoire overlay is currently enabled/shown.
  bool is_enabled{ false };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_GRIMOIRE_HPP__
