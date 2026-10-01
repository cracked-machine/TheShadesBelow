#ifndef SRC_SYSTEMS_SHOPSYSTEM_HPP__
#define SRC_SYSTEMS_SHOPSYSTEM_HPP__

#include <Components/Shop/Inventory.hpp>
#include <Events/BuyShopItemEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sys
{

//! @brief Handles the shop's inventory generation, shopkeeper interaction and item purchases.
class ShopSystem : public BaseSystem
{
public:
  //! @brief Construct a new Shop System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  //! @param scenemanager_event_dispatcher
  ShopSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, entt::dispatcher &scenemanager_event_dispatcher )
      : Game::Sys::BaseSystem( reg, window, sound_bank ),
        m_scenemanager_event_dispatcher( scenemanager_event_dispatcher )
  {
    get_systems_event_queue().sink<Events::BuyShopItemEvent>().connect<&ShopSystem::on_buy_shop_item>( this );
  }

  //! @brief Create an inventory component in the registry.
  //! @param inventory_entt The entity that will own the component
  void create_shop_inventory();

  //! @brief Pick a random "sprite.item." sprite and price and insert in the inventory component
  //! @param shop_inventory_cmp The inventory component
  void add_shop_inventory_item( Cmp::Shop::Inventory &shop_inventory_cmp );

  //! @brief Check for collisions with the exit
  void check_exit_collision();

  //! @brief If player has walked up to shop keeper and triggered shop overlay
  //! @param shopkeeper_pos
  //! @return true if the player is colliding with the shopkeeper
  //! @return false otherwise
  bool check_shopkeeper_collision( sf::Vector2f shopkeeper_pos );

  //! @brief Deduct player wealth, transfer item to player, replace item in shop inventory
  //! @param item_idx
  void buy_shop_item( uint8_t item_idx );

  //! @brief Intercepts an event, calls `buy_shop_item` function.
  //! @param ev
  void on_buy_shop_item( const Events::BuyShopItemEvent &ev ) { buy_shop_item( ev.m_item_idx ); }

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  // const Cmp::Shop::Inventory::Config &get_inventory_config() { return m_shop_inventory_config; }

private:
  //! @brief Dispatcher reference for scene management events
  entt::dispatcher &m_scenemanager_event_dispatcher;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_SHOPSYSTEM_HPP__
