#include <algorithm>
#include <cmath>
#include <optional>

#include <Audio/SoundBank.hpp>
#include <Components/Exit.hpp>
#include <Components/Inventory/FlashUIWealth.hpp>
#include <Components/Persistent/PaidHealAmount.hpp>
#include <Components/Persistent/ShopMaxItems.hpp>
#include <Components/Persistent/ShopMaxPrice.hpp>
#include <Components/Persistent/ShopMinPrice.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/RectBounds.hpp>
#include <Components/SceneSettings/CurrentScene.hpp>
#include <Components/Shop/Inventory.hpp>
#include <Components/Spring/HealingSpringMultiBlock.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Events/AddInventoryEvent.hpp>
#include <Events/DropInventoryEvent.hpp>
#include <Factory/ParticleFactory.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/ShopSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

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
    if ( not add_shop_inventory_item( shop_inventory_cmp ) ) break;
  }

  auto shop_inventory_entt = reg().create();
  reg().emplace_or_replace<Cmp::Shop::Inventory>( shop_inventory_entt, shop_inventory_cmp );
  Factory::Particle::sync_flames_for_shop_inventory( reg() );
}

int32_t ShopSystem::shop_price( int base_price )
{
  // luck [0, 100] shifts the fluctuation window: 0 luck -> [0%, +20%], 50 luck -> [-20%, +20%], 100 luck -> [-20%, 0%]
  const float luck_ratio = static_cast<float>( std::clamp( Utils::Player::get_stats( reg() ).luck(), 0, 100 ) ) / 100.f;
  const float bias = kPriceFluctuation - ( 2.f * kPriceFluctuation * luck_ratio );
  const float min_fluctuation = std::max( -kPriceFluctuation, bias - kPriceFluctuation );
  const float max_fluctuation = std::min( kPriceFluctuation, bias + kPriceFluctuation );
  const float fluctuation = Cmp::RandomFloat( min_fluctuation, max_fluctuation ).gen();
  return static_cast<int32_t>( std::lround( static_cast<float>( base_price ) * ( 1.f + fluctuation ) ) );
}

bool ShopSystem::add_shop_inventory_item( Cmp::Shop::Inventory &shop_inventory_cmp, const std::optional<Sys::ItemKey> &exclude )
{
  auto item_types = Sys::ItemStore::instance().get_all( Sys::ItemStore::ItemFilter::SHOP_ONLY );
  // keep the stock unique: never offer an item type that is already in the shop
  std::erase_if( item_types, [&shop_inventory_cmp]( const Sys::ItemKey &type )
                 { return std::ranges::any_of( shop_inventory_cmp.m_slots, [&type]( const auto &slot ) { return slot.first == type; } ); } );
  // avoid the excluded type (e.g. the item just bought), unless it is the only one left to keep the shop full
  if ( exclude and std::ranges::any_of( item_types, [&exclude]( const Sys::ItemKey &type ) { return type != *exclude; } ) )
  {
    std::erase( item_types, *exclude );
  }
  if ( item_types.empty() )
  {
    SPDLOG_WARN( "No unique shop items left to stock ({} slots filled)", shop_inventory_cmp.m_slots.size() );
    return false;
  }
  Cmp::RandomInt item_picker( 0, static_cast<int>( item_types.size() ) - 1 );
  const auto &selected_item_type = item_types.at( item_picker.gen() );
  const int32_t price = shop_price( Sys::ItemStore::instance().get( selected_item_type ).get_shop_base_price() );
  SPDLOG_INFO( "Adding shop item - {} - for {}", selected_item_type, price );
  shop_inventory_cmp.m_slots.emplace_back( selected_item_type, price );
  return true;
}

void ShopSystem::check_exit_collision()
{
  auto player_pos_cmp = Utils::Player::get_position( reg() );
  auto door_view = reg().view<Cmp::Exit, Cmp::Position>();
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [door_entity, door_pos_cmp] : door_view.each() )
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

    std::optional<Sys::ItemKey> bought_item;

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
      get_systems_event_queue().trigger( Events::AddInventoryEvent( item ) );
      // check if the player inventory has a torch, if so light it up!
      Factory::Particle::add_flame_for_player_inventory_slot( m_reg );

      // delete item from shop inventory, its replacement must be a different item
      bought_item = item;
      auto &slots = inventory_cmp.m_slots;
      auto it = std::next( slots.begin(), i );
      slots.erase( it );

      break;
    }

    // ensure that the shop is always fully stocked.
    while ( inventory_cmp.m_slots.size() < static_cast<size_t>( inventory_cmp.m_config.max_items ) )
    {
      if ( not add_shop_inventory_item( inventory_cmp, bought_item ) ) break;
    }
  }

  // the stock may have gained or lost a torch
  Factory::Particle::sync_flames_for_shop_inventory( reg() );
}

bool ShopSystem::is_heal_menu_open( entt::registry &reg )
{
  if ( not Utils::is_current_scene( reg, Cmp::SceneSettings::SceneId::BARBER_SURGEON ) ) return false;
  for ( auto [spring_entt, spring_mb_cmp] : reg.view<Cmp::HealingSpringMultiBlock>().each() )
  {
    if ( Utils::Player::is_player_near( reg, spring_mb_cmp ) ) return true;
  }
  return false;
}

bool ShopSystem::can_afford_heal() { return Utils::Player::get_wealth( reg() ).wealth >= kHealGoldCost; }

bool ShopSystem::buy_heal( HealStat stat )
{
  const int cost = kHealGoldCost;
  if ( not can_afford_heal() )
  {
    SPDLOG_INFO( "Not enough gold to buy a heal (need {})", cost );
    m_sound_bank.get_effect( "crypt_locked" ).play();
    return false;
  }

  Utils::Player::get_wealth( reg() ).wealth -= cost;
  // signal UI to flash
  reg().emplace_or_replace<Cmp::FlashUIWealth>( reg().create() );
  m_sound_bank.get_effect( "get_key" ).play();

  apply_heal( stat );
  SPDLOG_INFO( "Bought a {} heal for {} gold", kHealStatLabels.at( static_cast<std::size_t>( stat ) ), cost );
  return true;
}

void ShopSystem::apply_heal( HealStat stat )
{
  const int amount = Sys::PersistSystem::get<Cmp::Persist::PaidHealAmount>( reg() ).get_value();
  Cmp::Stats::Health health{};
  Cmp::Stats::Fear fear{};
  Cmp::Stats::Despair despair{};
  Cmp::Stats::Infamy infamy{};
  switch ( stat )
  {
    case HealStat::HEALTH:
      health.value = amount;
      break;
    case HealStat::FEAR:
      fear.value = -amount;
      break;
    case HealStat::DESPAIR:
      despair.value = -amount;
      break;
    case HealStat::INFAMY:
      infamy.value = -amount;
      break;
  }
  Utils::Player::get_stats( reg() ).apply( Cmp::BaseAction( health, fear, despair, infamy, {}, {} ) );
}

} // namespace Game::Sys