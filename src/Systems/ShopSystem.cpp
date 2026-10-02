#include <Components/Exit.hpp>
#include <Components/Persistent/ShopMaxItems.hpp>
#include <Components/Persistent/ShopMaxPrice.hpp>
#include <Components/Persistent/ShopMinPrice.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Shop/Inventory.hpp>
#include <Events/DropInventoryEvent.hpp>
#include <Factory/PlayerFactory.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/ShopSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Utils/Player.hpp>

namespace Game::Sys
{

void ShopSystem::create_shop_inventory()
{
  Cmp::Shop::Inventory shop_inventory_cmp;
  shop_inventory_cmp.m_config.max_items = Sys::PersistSystem::get<Cmp::Persist::ShopMaxItems>( m_reg ).get_value();
  shop_inventory_cmp.m_config.min_price = Sys::PersistSystem::get<Cmp::Persist::ShopMinPrice>( m_reg ).get_value();
  shop_inventory_cmp.m_config.max_price = Sys::PersistSystem::get<Cmp::Persist::ShopMaxPrice>( m_reg ).get_value();

  for ( auto _ : std::views::iota( 0, shop_inventory_cmp.m_config.max_items ) )
  {
    add_shop_inventory_item( shop_inventory_cmp );
  }

  auto shop_inventory_entt = reg().create();
  reg().emplace_or_replace<Cmp::Shop::Inventory>( shop_inventory_entt, shop_inventory_cmp );
}

void ShopSystem::add_shop_inventory_item( Cmp::Shop::Inventory &shop_inventory_cmp )
{
  auto item_types = Sys::ItemStore::instance().get_all();
  Cmp::RandomInt item_picker( 0, static_cast<int>( item_types.size() ) - 1 );
  Cmp::RandomInt price_picker( shop_inventory_cmp.m_config.min_price, shop_inventory_cmp.m_config.max_price );
  auto selected_item = item_picker.gen();
  auto price = price_picker.gen();
  SPDLOG_INFO( "Adding shop item - {} - for {}", item_types.at( selected_item ), price );
  shop_inventory_cmp.m_slots.emplace_back( item_types.at( selected_item ), price );
}

void ShopSystem::check_exit_collision()
{
  auto player_pos_cmp = Utils::Player::get_position( reg() );
  auto door_view = reg().view<Cmp::Exit, Cmp::Position>();
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [door_entity, door_cmp, door_pos_cmp] : door_view.each() )
  {
    // optimize: skip if not visible
    if ( !Utils::is_visible_in_view( view_bounds, door_pos_cmp ) ) continue;

    auto decreased_entrance_bounds = Cmp::RectBounds::scaled( door_pos_cmp.position, door_pos_cmp.size, 0.1f,
                                                              Cmp::RectBounds::ScaleAxis::XY ); // shrink entrance bounds slightly for better UX

    if ( not player_pos_cmp.findIntersection( decreased_entrance_bounds.getBounds() ) ) continue;

    SPDLOG_INFO( "check_exit_collision: Player exiting Shop to graveyard at position ({}, {})", player_pos_cmp.position.x,
                 player_pos_cmp.position.y );
    m_scenemanager_event_dispatcher.enqueue<Events::SceneManagerEvent>( Events::SceneManagerEvent::Type::EXIT_SHOP );
  }
}

bool ShopSystem::check_shopkeeper_collision( sf::Vector2f shopkeeper_pos )
{
  bool result = false;
  auto shopkeeper_hitbox = Cmp::RectBounds::scaled( shopkeeper_pos, Constants::kGridSizePxF, 3.f );
  auto player_pos = Utils::Player::get_position( reg() );
  if ( not shopkeeper_hitbox.findIntersection( player_pos ) ) return result;
  return result = true;
}

void ShopSystem::buy_shop_item( uint8_t item_idx )
{
  auto inventory_view = reg().view<Cmp::Shop::Inventory>().each();
  for ( auto [inventory_entt, inventory_cmp] : inventory_view )
  {
    if ( not inventory_cmp.is_enabled ) continue;
    if ( item_idx == 0 ) return;

    // auto && required: std::views::enumerate returns a temporary tuple, auto & cannot bind to it
    for ( auto &&[i, slot] : std::views::enumerate( inventory_cmp.m_slots ) )
    {
      // selections presented to the user are not zero-indexed so temp bump the idx when checking if it matches user selection
      if ( i + 1 != item_idx ) continue;

      // Found the inventory slot selected by user
      auto &[item, price] = slot;

      SPDLOG_INFO( "Found slot {}: {} - {}", item_idx, item, price );

      // check if player has enough money
      auto &player_wealth = Utils::Player::get_wealth( reg() );
      if ( player_wealth.wealth < static_cast<int32_t>( price ) ) { break; }
      player_wealth.wealth -= price;

      get_systems_event_queue().trigger( Events::DropInventoryEvent() );

      // add new carryitem into player inventory
      Factory::Player::add_inventory( reg(), item );

      // delete item from shop inventory
      auto &slots = inventory_cmp.m_slots;
      auto it = std::next( slots.begin(), i );
      slots.erase( it );

      // replace the shop stock
      add_shop_inventory_item( inventory_cmp );
      break;
    }

    // ensure that the shop is always fully stocked.
    while ( inventory_cmp.m_slots.size() < static_cast<size_t>( inventory_cmp.m_config.max_items ) )
    {
      add_shop_inventory_item( inventory_cmp );
    }
  }
}

} // namespace Game::Sys