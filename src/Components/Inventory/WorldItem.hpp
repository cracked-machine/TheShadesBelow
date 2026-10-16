#ifndef SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__
#define SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <SFML/System/Time.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <typeindex>
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

  //! @brief Construct a new, empty World Item object.
  WorldItem() = default;
  //! @brief Construct a new World Item object.
  //! @param item_type Item identifier, e.g. "item.pickaxe". See res/json/sprite_metadata.json.
  //! @param sprite_type The sprite used to render this item.
  //! @param expiry Time for the item to fully expire. Zero means this item does not expire.
  //! @param wear_location Where the item's wear level is active.
  WorldItem( Sys::ItemKey item_type, Sys::SpriteKey sprite_type, std::string sacrifice_output, bool is_sacrificial, int pickaxe_dmg, int axe_dmg,
             int shovel_dmg, int bow_dmg, sf::Time expiry = sf::Time::Zero, Location wear_location = Location::NONE,
             Consumable consumable = Consumable::NONE )
      : item_type( std::move( item_type ) ),
        sprite_type( std::move( sprite_type ) ),
        m_sacrifice_output( std::move( sacrifice_output ) ),
        m_is_sacrificial( is_sacrificial ),
        m_pickaxe_damage( pickaxe_dmg ),
        m_axe_damage( axe_dmg ),
        m_shovel_damage( shovel_dmg ),
        m_bow_damage( bow_dmg ),
        m_expiry( expiry ),
        m_wear_location( wear_location ),
        m_consumable( consumable )
  {
  }

  //! @brief Item identifier, e.g. "item.pickaxe". See res/json/sprite_metadata.json.
  Sys::ItemKey item_type;

  //! @brief The associated sprite. Imported from items.json.
  Sys::SpriteKey sprite_type;

  //! @brief Pairs a stat-modifier action with the timestamp/duration used to schedule its re-application.
  struct ActionTimePair
  {
    //! @brief The stat modifier to apply.
    BaseAction action;
    //! @brief The time associated with the action (e.g. last-applied time or remaining duration).
    sf::Time time;
  };

  template <typename ActionT>
  auto &at()
  {
    return actions.at( std::type_index( typeid( ActionT ) ) );
  }
  template <typename ActionT>
  const auto &at() const
  {
    return actions.at( std::type_index( typeid( ActionT ) ) );
  }
  //! @brief Add an action keyed by its concrete type. Must be a template: taking BaseAction by value would slice
  //! the argument and key every action under typeid(BaseAction).
  template <typename ActionT>
  void emplace( ActionT action )
  {
    actions.emplace( std::type_index( typeid( ActionT ) ), ActionTimePair{ action, sf::Time::Zero } );
  }
  auto begin() { return actions.begin(); }
  auto end() { return actions.end(); }

  template <typename ActionT>
  BaseAction get_action() const
  {
    return actions.at( std::type_index( typeid( ActionT ) ) ).action;
  }

  sf::Time expiry() const { return m_expiry; }

  //! @brief Whether the item carries a Cmp::Inventory::WearLevel.
  bool has_wear() const { return m_wear_location != Location::NONE; }
  //! @brief Whether the item's wear level is active while it is in the world.
  bool wears_in_world() const { return m_wear_location == Location::WORLD or m_wear_location == Location::BOTH; }
  //! @brief Whether the item's wear level is active while it is in the player inventory.
  bool wears_in_inventory() const { return m_wear_location == Location::INVENTORY or m_wear_location == Location::BOTH; }

  Sys::ItemKey get_sacrifice_output() const { return m_sacrifice_output; }

  bool is_sacrificial() const { return m_is_sacrificial; }

  int get_pickaxe_damage() const { return m_pickaxe_damage; }
  int get_axe_damage() const { return m_axe_damage; }
  int get_shovel_damage() const { return m_shovel_damage; }
  int get_bow_damage() const { return m_bow_damage; }

  Consumable consumable_type() { return m_consumable; }

private:
  //! @brief The action and its effects that can be applied to the player. Imported from items.json.
  std::unordered_map<std::type_index, ActionTimePair> actions;

  //! @brief The item that will spawn when this item is sacrificed. Empty string is a no op.
  Sys::ItemKey m_sacrifice_output;

  //! @brief true if this item can be  sacrificed at the altar
  bool m_is_sacrificial;

  //! @brief damage done to pickaxe by this item
  int m_pickaxe_damage;

  //! @brief damage done to axe by this item
  int m_axe_damage;

  //! @brief damage done to shovel by this item
  int m_shovel_damage;

  //! @brief damage done to bow by this item
  int m_bow_damage;

  //! @brief Expiry for the item. Zero means this item does not exire. Imported from items.json.
  sf::Time m_expiry;

  //! @brief Where the item's wear level is active. Imported from items.json.
  Location m_wear_location{ Location::NONE };

  Consumable m_consumable{ Consumable::NONE };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__
