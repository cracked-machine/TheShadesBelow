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

  //! @brief Construct a new, empty World Item object.
  WorldItem() = default;
  //! @brief Construct a new World Item object.
  //! @param item_type Item identifier, e.g. "item.pickaxe". See res/json/sprite_metadata.json.
  //! @param sprite_type The sprite used to render this item.
  //! @param expiry Time for the item to fully expire. Zero means this item does not expire.
  //! @param wear_location Where the item's wear level is active.
  WorldItem( Sys::ItemKey item_type, Sys::SpriteKey sprite_type, sf::Time expiry = sf::Time::Zero, Location wear_location = Location::NONE )
      : item_type( std::move( item_type ) ),
        sprite_type( std::move( sprite_type ) ),
        m_expiry( expiry ),
        m_wear_location( wear_location )
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
  BaseAction get_action()
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

private:
  //! @brief The action and its effects that can be applied to the player. Imported from items.json.
  std::unordered_map<std::type_index, ActionTimePair> actions;

  //! @brief Expiry for the item. Zero means this item does not exire. Imported from items.json.
  sf::Time m_expiry;

  //! @brief Where the item's wear level is active. Imported from items.json.
  Location m_wear_location{ Location::NONE };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__
