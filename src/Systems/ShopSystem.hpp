#ifndef SRC_SYSTEMS_SHOPSYSTEM_HPP__
#define SRC_SYSTEMS_SHOPSYSTEM_HPP__

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

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
  //! @param exclude Item type to avoid if any other unstocked type is available, e.g. the item just bought
  //! @return false if every shop item type is already stocked, so no item was added
  bool add_shop_inventory_item( Cmp::Shop::Inventory &shop_inventory_cmp, const std::optional<Sys::ItemKey> &exclude = std::nullopt );

  //! @brief Shop price for an item: base_price fluctuated by up to +/-kPriceFluctuation, biased by player luck.
  //! Low luck biases the fluctuation towards +kPriceFluctuation, high luck towards -kPriceFluctuation.
  //! @param base_price The item's "shop.base_price" from items.json
  //! @return The randomised shop price
  int32_t shop_price( int base_price );

  //! @brief Maximum fraction the shop price may fluctuate from the base price, in either direction.
  static constexpr float kPriceFluctuation = 0.2f;

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

  // const Cmp::Shop::Inventory::Config &get_inventory_config() { return m_shop_inventory_config; }

  //! @brief The stats the barber surgeon can heal, in the order they are listed in the heal menu.
  enum class HealStat {
    //! @brief Restore health
    HEALTH,
    //! @brief Reduce fear
    FEAR,
    //! @brief Reduce despair
    DESPAIR,
    //! @brief Reduce infamy
    INFAMY
  };

  //! @brief Heal menu row labels, indexed by HealStat
  static constexpr std::array<std::string_view, 4> kHealStatLabels{ "Health", "Fear", "Despair", "Infamy" };

  //! @brief Gold cost of one heal at the barber surgeon
  static constexpr int kHealGoldCost = 10;

  //! @brief True if the heal menu should show: the current scene is the barber surgeon, and the player
  //! is near the fountain (Cmp::HealingSpringMultiBlock).
  //! @param reg
  [[nodiscard]] static bool is_heal_menu_open( entt::registry &reg );

  //! @brief True if the player has enough gold for one heal.
  [[nodiscard]] bool can_afford_heal();

  //! @brief Pay gold for one heal and apply it (Cmp::Persist::PaidHealAmount) to `stat`: health goes up,
  //! fear/despair/infamy go down. Plays a denied sound instead if the player can't afford it.
  //! @param stat the stat to heal
  //! @return true if the heal was bought
  bool buy_heal( HealStat stat );

private:
  //! @brief Apply one paid heal (Cmp::Persist::PaidHealAmount) to `stat`.
  //! @param stat
  void apply_heal( HealStat stat );

  //! @brief Dispatcher reference for scene management events
  entt::dispatcher &m_scenemanager_event_dispatcher;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_SHOPSYSTEM_HPP__
