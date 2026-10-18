#ifndef SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__
#define SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <SFML/System/Time.hpp>
#include <Systems/Stores/StoreKey.hpp>

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
  //! @brief Where an item's Cmp::Inventory::WearLevel is active. Imported from items.json.
  enum class Location {
    //! @brief The item has no wear level at all.
    NONE,
    //! @brief The item only wears while it is in the world.
    WORLD,
    //! @brief The item only wears while it is in the player inventory.
    INVENTORY,
    //! @brief The item wears both in the world and in the player inventory.
    BOTH
  };

  //! @brief Determines if the item can be eaten, drunk or neither.
  enum class Consumable {
    //! @brief The item cannot be consumed.
    NONE,
    //! @brief The item can be eaten by the player.
    EDIBLE,
    //! @brief The item can be drunk by the player.
    DRINKABLE
  };

  //! @brief A special altar effect triggered by sacrificing the item.
  enum class SacrificeSpecial {
    //! @brief No special effect.
    NONE,
    //! @brief Light an altar candle (relics).
    ALTAR,
    //! @brief Banish ghosts, or gain an extra life at a crypt altar (witches jar).
    BANISH,
    //! @brief Unlock the crypt key (exit key).
    CRYPT
  };

  //! @brief Whether the item can be sacrificed and what it is exchanged for. When enabled, exactly one of
  //!        item, wealth or special is set.
  struct Sacrifice
  {
    //! @brief Whether the item can be sacrificed at an altar.
    bool enabled{ false };
    //! @brief The item that spawns when this item is sacrificed. Empty string if unused.
    Sys::ItemKey item;
    //! @brief The wealth gained when this item is sacrificed. Zero if unused.
    int wealth{ 0 };
    //! @brief The special altar effect. NONE if unused.
    SacrificeSpecial special{ SacrificeSpecial::NONE };
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
  //! @param sacrifice Whether this item can be sacrificed at the altar, and its reward.
  //! @param forage_output The item gained when this plant is foraged (empty hand or gloves). Empty string is a no op.
  //! @param axe_output The item dropped when this plant is chopped with an axe. Empty string is a no op.
  //! @param pickaxe_dmg Damage done to a pickaxe by this item.
  //! @param axe_dmg Damage done to an axe by this item.
  //! @param shovel_dmg Damage done to a shovel by this item.
  //! @param bow_dmg Damage done to a bow by this item.
  //! @param expiry Time for the item to fully expire. Zero means this item does not expire.
  //! @param wear_location Where the item's wear level is active.
  //! @param consumable Whether the item can be eaten, drunk or neither.
  WorldItem( Sys::ItemKey item_type, Sys::SpriteKey sprite_type, Sacrifice sacrifice, std::string forage_output, std::string axe_output, int pickaxe_dmg,
             int axe_dmg, int shovel_dmg, int bow_dmg, sf::Time expiry = sf::Time::Zero, Location wear_location = Location::NONE,
             Consumable consumable = Consumable::NONE )
      : item_type( std::move( item_type ) ),
        sprite_type( std::move( sprite_type ) ),
        m_sacrifice( std::move( sacrifice ) ),
        m_forage_output( std::move( forage_output ) ),
        m_axe_output( std::move( axe_output ) ),
        m_pickaxe_damage( pickaxe_dmg ),
        m_axe_damage( axe_dmg ),
        m_shovel_damage( shovel_dmg ),
        m_bow_damage( bow_dmg ),
        m_expiry( expiry ),
        m_wear_location( wear_location ),
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

  //! @brief Get a copy of the action keyed by ActionT. Throws std::out_of_range if absent.
  template <typename ActionT>
  BaseAction get_action() const
  {
    return m_actions.at( std::type_index( typeid( ActionT ) ) ).action;
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

  //! @brief Time for the item to fully expire. Zero means this item does not expire.
  sf::Time expiry() const { return m_expiry; }

  //! @brief Whether the item carries a Cmp::Inventory::WearLevel.
  bool has_wear() const { return m_wear_location != Location::NONE; }

  //! @brief Whether the item's wear level is active while it is in the world.
  bool wears_in_world() const { return m_wear_location == Location::WORLD or m_wear_location == Location::BOTH; }

  //! @brief Whether the item's wear level is active while it is in the player inventory.
  bool wears_in_inventory() const { return m_wear_location == Location::INVENTORY or m_wear_location == Location::BOTH; }

  //! @brief The item that spawns when this item is sacrificed. Empty string if unused.
  Sys::ItemKey get_sacrifice_item() const { return m_sacrifice.item; }

  //! @brief The wealth gained when this item is sacrificed. Zero if unused.
  int get_sacrifice_wealth() const { return m_sacrifice.wealth; }

  //! @brief The special altar effect triggered when this item is sacrificed. NONE if unused.
  SacrificeSpecial get_sacrifice_special() const { return m_sacrifice.special; }

  //! @brief The item gained when this plant is foraged (empty hand or gloves). Empty string is a no op.
  Sys::ItemKey get_forage_output() const { return m_forage_output; }

  //! @brief The item dropped when this plant is chopped with an axe. Empty string is a no op.
  Sys::ItemKey get_axe_output() const { return m_axe_output; }

  //! @brief Whether this item can be sacrificed at the altar.
  bool is_sacrificial() const { return m_sacrifice.enabled; }

  //! @brief Damage done to the corresponding tool by this item.
  int get_pickaxe_damage() const { return m_pickaxe_damage; }
  int get_axe_damage() const { return m_axe_damage; }
  int get_shovel_damage() const { return m_shovel_damage; }
  int get_bow_damage() const { return m_bow_damage; }

  //! @brief Whether the item can be eaten, drunk or neither.
  Consumable consumable_type() const { return m_consumable; }

  //! @brief Item identifier, e.g. "item.pickaxe". See res/json/sprite_metadata.json.
  Sys::ItemKey item_type;

  //! @brief The associated sprite. Imported from items.json.
  Sys::SpriteKey sprite_type;

private:
  //! @brief The actions and their effects that can be applied to the player. Imported from items.json.
  std::unordered_map<std::type_index, ActionTimePair> m_actions;

  //! @brief Whether this item can be sacrificed at the altar, and its reward. Imported from items.json.
  Sacrifice m_sacrifice;

  //! @brief The item gained when this plant is foraged (empty hand or gloves). Empty string is a no op.
  Sys::ItemKey m_forage_output;

  //! @brief The item dropped when this plant is chopped with an axe. Empty string is a no op.
  Sys::ItemKey m_axe_output;

  //! @brief Damage done to a pickaxe by this item.
  int m_pickaxe_damage{ 0 };

  //! @brief Damage done to an axe by this item.
  int m_axe_damage{ 0 };

  //! @brief Damage done to a shovel by this item.
  int m_shovel_damage{ 0 };

  //! @brief Damage done to a bow by this item.
  int m_bow_damage{ 0 };

  //! @brief Expiry for the item. Zero means this item does not expire. Imported from items.json.
  sf::Time m_expiry;

  //! @brief Where the item's wear level is active. Imported from items.json.
  Location m_wear_location{ Location::NONE };

  //! @brief Whether the item can be eaten, drunk or neither. Imported from items.json.
  Consumable m_consumable{ Consumable::NONE };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__
