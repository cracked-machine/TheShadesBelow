#ifndef SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__
#define SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__

#include <Components/Altar/Kind.hpp>
#include <Components/Random.hpp>
#include <Components/Stats/ActionSet.hpp>
#include <Components/Stats/BaseActionFwd.hpp>
#include <SFML/System/Time.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <optional>
#include <string>
#include <typeindex>
#include <typeinfo>
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

  //! @brief Construct a new, empty World Item object.
  WorldItem() = default;

  //! @brief Construct a new World Item object.
  //! @param item_type Item identifier, e.g. "item.shovel". See res/json/sprite_metadata.json.
  //! @param sprite_type The sprite used to render this item.
  //! @param sprite_index The frame of sprite_type to draw (items.json sprite.index).
  //! @param sprite_random_count If non-zero, each new sprite of this item is a random frame in [0, sprite_random_count) instead
  //! of sprite_index (items.json sprite.random; the frame count of sprite_type).
  //! @param pickaxe_dmg Damage done to a pickaxe by this item.
  //! @param axe_dmg Damage done to an axe by this item.
  //! @param shovel_dmg Damage done to a shovel by this item.
  //! @param bow_dmg Damage done to a bow by this item.
  //! @param consumable Whether the item can be eaten, drunk or neither.
  WorldItem( Sys::ItemKey item_type, Sys::SpriteKey sprite_type, std::size_t sprite_index, std::size_t sprite_random_count, int pickaxe_dmg,
             int axe_dmg, int shovel_dmg, int bow_dmg, Consumable consumable = Consumable::NONE )
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

  //! @brief Get the action/time pair keyed by ActionT. Every action is optional in items.json: if absent, a zeroed
  //! (no effect) action is inserted, so the returned timer is owned by this item. Use has_action() beforehand if
  //! "absent" must be told apart from "present but no effect".
  //! @note The caller includes the ActionT header (e.g. CarryAction.hpp), which completes ActionTimePair.
  template <typename ActionT>
  ActionTimePair &at()
  {
    return m_actions.at( std::type_index( typeid( ActionT ) ) );
  }

  //! @brief Get the action/time pair keyed by ActionT, or a shared zeroed (no effect) pair if absent.
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

  // The getters below deduce their return types: a spelled-out BaseAction / std::optional<ActionReward> would need
  // BaseAction.hpp here. A missing action reads as no_action(), whose reward, rewards, spawn_npc and wear are empty.

  //! @brief Get a copy of the action keyed by ActionT (a Cmp::BaseAction), or a zeroed (no effect) action if absent.
  template <typename ActionT>
  [[nodiscard]] auto get_action() const
  {
    return at<ActionT>().action;
  }

  //! @brief Get the reward of the action keyed by ActionT.
  //! @return std::optional<ActionReward>: the reward, or std::nullopt if the item has no such action or the action has no reward.
  template <typename ActionT>
  [[nodiscard]] auto get_reward() const
  {
    return at<ActionT>().action.reward();
  }

  //! @brief Get all rewards of the action keyed by ActionT.
  //! @return ActionRewards: the rewards, empty if the item has no such action or the action has no reward.
  template <typename ActionT>
  [[nodiscard]] auto get_rewards() const
  {
    return at<ActionT>().action.rewards();
  }

  //! @brief Get the spawn npc effect of the action keyed by ActionT.
  //! @return std::optional<SpawnNpcEffect>: the effect, or std::nullopt if the item has no such action or the action has
  //! no npc spawn effect.
  template <typename ActionT>
  [[nodiscard]] auto get_spawn_npc_effect() const
  {
    return at<ActionT>().action.side_effects().spawn_npc;
  }

  //! @brief Get the wear of the action keyed by ActionT ("wear" in items.json).
  //! @return std::optional<ActionWear>: the wear, or std::nullopt if the item has no such action or the action has no wear.
  template <typename ActionT>
  [[nodiscard]] auto get_wear() const
  {
    return at<ActionT>().action.wear();
  }

  //! @brief Add an action keyed by its concrete type. Must be a template: keying by the BaseAction argument's type
  //! would key every action under typeid(BaseAction).
  template <typename ActionT>
  void emplace( const ActionT &action )
  {
    m_actions.emplace( std::type_index( typeid( ActionT ) ), action );
  }

  //! @brief All of the item's actions.
  [[nodiscard]] const ActionMap &actions() const { return m_actions.map(); }

  //! @brief Whether the item carries a Cmp::Inventory::WearLevel, i.e. any of its actions has a "wear" block in items.json.
  bool has_wear() const;

  //! @brief Whether the item wears while it is in the world ("spawn_action.wear" in items.json).
  bool wears_in_world() const;

  //! @brief Time for the item to fully expire while it is in the world. Zero means it does not expire there.
  sf::Time world_expiry() const;

  //! @brief Whether the item wears while it is in the player inventory ("carry_action.wear" in items.json).
  bool wears_in_inventory() const;

  //! @brief Time for the item to fully expire while it is in the player inventory. Zero means it does not expire there.
  sf::Time inventory_expiry() const;

  //! @brief Whether the item only wears in the inventory while it is actively in use (e.g. flute playing, broom flying).
  bool wears_only_when_active() const;

  //! @brief The reward for offering this item at the given kind of altar ("offering_action.reward" in items.json).
  //! @param location The kind of altar the item is offered at.
  //! @return The reward, or std::nullopt if the item can't be offered at that kind of altar.
  std::optional<ActionReward> get_offering_reward( Altar::Kind location ) const;

  //! @brief The blood this item adds to a graveyard altar's quota ("offering_action.reward.blood" in items.json).
  //! @return The blood, or 0 if the item is not a blood offering.
  int get_blood_reward() const;

  //! @brief The npc spawn effect for offering this item at the altar ("side_effects.spawn_npc" in items.json).
  //! @return The reward, or std::nullopt if the item can't be offered.
  std::optional<SpawnNpcEffect> get_offering_npc_spawn_effect() const;

  //! @brief Whether this item can be offered at the altar, i.e. its offering_action has any effect: a non zero stat
  //! delta, a toxidrome, a side effect or a reward.
  bool is_offerable() const;

  //! @brief The plant's forage item, see "forage_action.reward.item" in items.json. Plants are no longer foraged:
  //! it identifies the forage item among the destroy_action drop pool (e.g. for the grimoire). Empty string if none.
  Sys::ItemKey get_forage_output() const;

  //! @brief The plant's own drop item, see "destroy_action.reward.item" in items.json. For a random drop pool this is
  //! the first entry that is neither the forage output nor the plant itself. Empty string if none.
  Sys::ItemKey get_drop_output() const;

  //! @brief Damage done to the corresponding tool by this item.
  int get_pickaxe_damage() const { return m_pickaxe_damage; }
  int get_axe_damage() const { return m_axe_damage; }
  int get_shovel_damage() const { return m_shovel_damage; }
  int get_bow_damage() const { return m_bow_damage; }

  //! @brief Number of shovel hits needed to destroy this plant, see "destroy_hits" in items.json. Defaults to 1.
  int get_destroy_hits() const { return m_destroy_hits; }
  void set_destroy_hits( int destroy_hits ) { m_destroy_hits = destroy_hits; }

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
  bool stings() const;

  //! @brief Item identifier, e.g. "item.shovel". See res/json/sprite_metadata.json.
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
  ActionSet m_actions;

  //! @brief Damage done to a pickaxe by this item.
  int m_pickaxe_damage{ 0 };

  //! @brief Damage done to an axe by this item.
  int m_axe_damage{ 0 };

  //! @brief Damage done to a shovel by this item.
  int m_shovel_damage{ 0 };

  //! @brief Damage done to a bow by this item.
  int m_bow_damage{ 0 };

  //! @brief Number of shovel hits needed to destroy this plant. Imported from items.json.
  int m_destroy_hits{ 1 };

  //! @brief Whether the item can be eaten, drunk or neither. Imported from items.json.
  Consumable m_consumable{ Consumable::NONE };

  //! @brief Whether the item can be stocked by the shop. Imported from items.json.
  bool m_shop{ false };

  //! @brief Shop base price, fluctuated +/-20% by luck to get the shop price. Imported from items.json.
  int m_shop_base_price{ 0 };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__
