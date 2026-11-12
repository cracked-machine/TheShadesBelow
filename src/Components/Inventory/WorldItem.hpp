#ifndef SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__
#define SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__

#include <Components/Random.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ForageAction.hpp>
#include <Components/Stats/OfferingAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <SFML/System/Time.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>

namespace Game::Cmp
{

//! @brief An item found in the world or placed in a players inventory slot.
//! @details This defines both the sprite type and the action effects associated with the item.
class WorldItem
{
public:
  //! @brief Determines if the item can be eaten, drunk or neither.
  enum class Consumable {
    //! @brief The item cannot be consumed.
    NONE,
    //! @brief The item can be eaten by the player.
    EDIBLE,
    //! @brief The item can be drunk by the player.
    DRINKABLE
  };

  //! @brief Pairs a stat-modifier action with the timestamp/duration used to schedule its re-application.
  struct ActionTimePair
  {
    //! @brief The stat modifier to apply.
    BaseAction action;
    //! @brief The time associated with the action (e.g. last-applied time or remaining duration).
    sf::Time time;
  };

  //! @brief Construct a new, empty World Item object.
  WorldItem() = default;

  //! @brief Construct a new World Item object.
  //! @param item_type Item identifier, e.g. "item.pickaxe". See res/json/sprite_metadata.json.
  //! @param sprite_type The sprite used to render this item.
  //! @param sprite_index The frame of sprite_type to draw (items.json sprite.index).
  //! @param sprite_random_count If non-zero, each new sprite of this item is a random frame in [0, sprite_random_count) instead
  //! of sprite_index (items.json sprite.random; the frame count of sprite_type).
  //! @param pickaxe_dmg Damage done to a pickaxe by this item.
  //! @param axe_dmg Damage done to an axe by this item.
  //! @param shovel_dmg Damage done to a shovel by this item.
  //! @param bow_dmg Damage done to a bow by this item.
  //! @param consumable Whether the item can be eaten, drunk or neither.
  WorldItem( Sys::ItemKey item_type, Sys::SpriteKey sprite_type, std::size_t sprite_index, std::size_t sprite_random_count, int pickaxe_dmg, int axe_dmg, int shovel_dmg, int bow_dmg,
             Consumable consumable = Consumable::NONE )
      : item_type( std::move( item_type ) ),
        sprite_type( std::move( sprite_type ) ),
        m_sprite_index( sprite_index ),
        m_sprite_random_count( sprite_random_count ),
        m_pickaxe_damage( pickaxe_dmg ),
        m_axe_damage( axe_dmg ),
        m_shovel_damage( shovel_dmg ),
        m_bow_damage( bow_dmg ),
        m_consumable( consumable )
  {
  }

  //! @brief Get the action/time pair keyed by ActionT. Throws std::out_of_range if absent.
  template <typename ActionT>
  ActionTimePair &at()
  {
    return m_actions.at( std::type_index( typeid( ActionT ) ) );
  }

  //! @brief Get the action/time pair keyed by ActionT. Throws std::out_of_range if absent.
  template <typename ActionT>
  const ActionTimePair &at() const
  {
    return m_actions.at( std::type_index( typeid( ActionT ) ) );
  }

  //! @brief Whether the item has an action keyed by ActionT.
  template <typename ActionT>
  [[nodiscard]] bool has_action() const
  {
    return m_actions.contains( std::type_index( typeid( ActionT ) ) );
  }

  //! @brief Get a copy of the action keyed by ActionT. Throws std::out_of_range if absent.
  template <typename ActionT>
  BaseAction get_action() const
  {
    return m_actions.at( std::type_index( typeid( ActionT ) ) ).action;
  }

  //! @brief Get the reward of the action keyed by ActionT.
  //! @return The reward, or std::nullopt if the item has no such action or the action has no reward.
  template <typename ActionT>
  [[nodiscard]] std::optional<ActionReward> get_reward() const
  {
    if ( not has_action<ActionT>() ) return std::nullopt;
    return at<ActionT>().action.reward();
  }

  //! @brief Get all rewards of the action keyed by ActionT.
  //! @return The rewards, empty if the item has no such action or the action has no reward.
  template <typename ActionT>
  [[nodiscard]] ActionRewards get_rewards() const
  {
    if ( not has_action<ActionT>() ) return {};
    return at<ActionT>().action.rewards();
  }

  //! @brief Get the spawn npc effect of the action keyed by ActionT.
  //! @return The npc spawn effect object, or std::nullopt if the item has no such action or the action has no npc spawn effect.
  template <typename ActionT>
  [[nodiscard]] std::optional<SpawnNpcEffect> get_spawn_npc_effect() const
  {
    if ( not has_action<ActionT>() ) return std::nullopt;
    return at<ActionT>().action.side_effects().spawn_npc;
  }

  //! @brief Get the wear of the action keyed by ActionT ("wear" in items.json).
  //! @return The wear, or std::nullopt if the item has no such action or the action has no wear.
  template <typename ActionT>
  [[nodiscard]] std::optional<ActionWear> get_wear() const
  {
    if ( not has_action<ActionT>() ) return std::nullopt;
    return at<ActionT>().action.wear();
  }

  //! @brief Add an action keyed by its concrete type. Must be a template: taking BaseAction by value would slice
  //! the argument and key every action under typeid(BaseAction).
  template <typename ActionT>
  void emplace( ActionT action )
  {
    m_actions.emplace( std::type_index( typeid( ActionT ) ), ActionTimePair{ action, sf::Time::Zero } );
  }

  //! @brief Iterators over the item's actions.
  auto begin() { return m_actions.begin(); }
  auto end() { return m_actions.end(); }

  //! @brief Whether the item carries a Cmp::Inventory::WearLevel, i.e. any of its actions has a "wear" block in items.json.
  bool has_wear() const
  {
    return std::ranges::any_of( m_actions, []( const auto &pair ) { return pair.second.action.wear().has_value(); } );
  }

  //! @brief Whether the item wears while it is in the world ("spawn_action.wear" in items.json).
  bool wears_in_world() const { return get_wear<SpawnAction>().has_value(); }

  //! @brief Time for the item to fully expire while it is in the world. Zero means it does not expire there.
  sf::Time world_expiry() const { return sf::seconds( get_wear<SpawnAction>().value_or( ActionWear{} ).expiry_seconds ); }

  //! @brief Whether the item wears while it is in the player inventory ("carry_action.wear" in items.json).
  bool wears_in_inventory() const { return get_wear<CarryAction>().has_value(); }

  //! @brief Time for the item to fully expire while it is in the player inventory. Zero means it does not expire there.
  sf::Time inventory_expiry() const { return sf::seconds( get_wear<CarryAction>().value_or( ActionWear{} ).expiry_seconds ); }

  //! @brief Whether the item only wears in the inventory while it is actively in use (e.g. flute playing, broom flying).
  bool wears_only_when_active() const { return get_wear<CarryAction>().value_or( ActionWear{} ).active_only; }

  //! @brief The reward for offering this item at the given kind of altar ("offering_action.reward" in items.json).
  //! @param location The kind of altar the item is offered at.
  //! @return The reward, or std::nullopt if the item can't be offered at that kind of altar.
  std::optional<ActionReward> get_offering_reward( Altar::Kind location ) const
  {
    if ( not has_action<OfferingAction>() ) return std::nullopt;
    return at<OfferingAction>().action.reward( location );
  }

  //! @brief The npc spawn effect for offering this item at the altar ("side_effects.spawn_npc" in items.json).
  //! @return The reward, or std::nullopt if the item can't be offered.
  std::optional<SpawnNpcEffect> get_offering_npc_spawn_effect() const { return get_spawn_npc_effect<OfferingAction>(); }

  //! @brief Whether this item can be offered at the altar, i.e. its offering_action has any effect: a non zero stat
  //! delta, a toxidrome, a side effect or a reward.
  bool is_offerable() const { return has_action<OfferingAction>() and not at<OfferingAction>().action.is_zero(); }

  //! @brief The item gained when this plant is foraged (empty hand or gloves), see "forage_action.reward.item" in
  //! items.json. Empty string is a no op.
  Sys::ItemKey get_forage_output() const
  {
    const auto reward = get_reward<ForageAction>();
    return reward ? reward->item : Sys::ItemKey();
  }

  //! @brief The item dropped when this plant is chopped with an axe, see "destroy_action.reward.item" in items.json.
  //! Empty string is a no op.
  Sys::ItemKey get_axe_output() const
  {
    const auto reward = get_reward<DestroyAction>();
    return reward ? reward->item : Sys::ItemKey();
  }

  //! @brief Damage done to the corresponding tool by this item.
  int get_pickaxe_damage() const { return m_pickaxe_damage; }
  int get_axe_damage() const { return m_axe_damage; }
  int get_shovel_damage() const { return m_shovel_damage; }
  int get_bow_damage() const { return m_bow_damage; }

  //! @brief Number of axe hits needed to chop this plant down, see "axe_hits" in items.json. Defaults to 1.
  int get_axe_hits() const { return m_axe_hits; }
  void set_axe_hits( int axe_hits ) { m_axe_hits = axe_hits; }

  //! @brief Whether the item can be eaten, drunk or neither.
  Consumable consumable_type() const { return m_consumable; }

  //! @brief Whether the item can be stocked by the shop, see "shop.enabled" in items.json. Defaults to false.
  bool is_shop_item() const { return m_shop; }
  void set_shop_item( bool shop ) { m_shop = shop; }

  //! @brief Shop base price, see "shop.base_price" in items.json. The shop price fluctuates +/-20% from this, biased by player luck.
  int get_shop_base_price() const { return m_shop_base_price; }
  void set_shop_base_price( int base_price ) { m_shop_base_price = base_price; }

  //! @brief Whether the item's spawn_action or carry_action damages the player (negative "health" in items.json), i.e.
  //! whether a Cmp::Inventory::StingDamage is added when the item is created.
  bool stings() const
  {
    return ( has_action<SpawnAction>() and at<SpawnAction>().action.health() < 0 ) or
           ( has_action<CarryAction>() and at<CarryAction>().action.health() < 0 );
  }

  //! @brief Item identifier, e.g. "item.pickaxe". See res/json/sprite_metadata.json.
  Sys::ItemKey item_type;

  //! @brief The associated sprite. Imported from items.json.
  Sys::SpriteKey sprite_type;

  //! @brief The frame of sprite_type to give a new sprite of this item, as set by items.json.
  //! @details sprite.random: a fresh random frame on every call. sprite.index: that frame. Neither: 0.
  //! Call once when creating the item's AnimData; the entity's AnimData then holds the frame (and travels with it, e.g. on
  //! pickup). For an icon redrawn every frame with no entity behind it, use icon_sprite_index() so it doesn't flicker.
  [[nodiscard]] std::size_t sprite_index() const
  {
    if ( m_sprite_random_count == 0 ) return m_sprite_index;
    return static_cast<std::size_t>( RandomInt( 0, static_cast<int>( m_sprite_random_count ) - 1 ).gen() );
  }

  //! @brief A stable frame for icons with no entity behind them (shop, grimoire, altar): sprite.index, else 0.
  [[nodiscard]] std::size_t icon_sprite_index() const { return m_sprite_index; }

  //! @brief The number of frames sprite_index() picks from when items.json sprite.random is set, else 0.
  [[nodiscard]] std::size_t sprite_random_count() const { return m_sprite_random_count; }

private:
  //! @brief items.json sprite.index. See sprite_index().
  std::size_t m_sprite_index{ 0 };

  //! @brief Frame count of sprite_type if items.json sprite.random is set, else 0. See sprite_index().
  std::size_t m_sprite_random_count{ 0 };

  //! @brief The actions and their effects that can be applied to the player. Imported from items.json.
  std::unordered_map<std::type_index, ActionTimePair> m_actions;

  //! @brief Damage done to a pickaxe by this item.
  int m_pickaxe_damage{ 0 };

  //! @brief Damage done to an axe by this item.
  int m_axe_damage{ 0 };

  //! @brief Damage done to a shovel by this item.
  int m_shovel_damage{ 0 };

  //! @brief Damage done to a bow by this item.
  int m_bow_damage{ 0 };

  //! @brief Number of axe hits needed to chop this plant down. Imported from items.json.
  int m_axe_hits{ 1 };

  //! @brief Whether the item can be eaten, drunk or neither. Imported from items.json.
  Consumable m_consumable{ Consumable::NONE };

  //! @brief Whether the item can be stocked by the shop. Imported from items.json.
  bool m_shop{ false };

  //! @brief Shop base price, fluctuated +/-20% by luck to get the shop price. Imported from items.json.
  int m_shop_base_price{ 0 };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__
