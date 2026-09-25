#ifndef SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__
#define SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <Sprites/SpriteMetaType.hpp>

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
  //! @brief Construct a new, empty World Item object.
  WorldItem() = default;
  //! @brief Construct a new World Item object.
  //! @param item_type Item identifier, e.g. "item.pickaxe". See res/json/sprite_metadata.json.
  //! @param sprite_type The sprite used to render this item.
  WorldItem( std::string item_type, Sprites::SpriteMetaType sprite_type )
      : item_type( std::move( item_type ) ),
        sprite_type( std::move( sprite_type ) )
  {
  }

  //! @brief Item identifier, e.g. "item.pickaxe". See res/json/sprite_metadata.json.
  std::string item_type;

  //! @brief The associated sprite
  Sprites::SpriteMetaType sprite_type;

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

private:
  //! @brief The action and its effects that can be applied to the player
  std::unordered_map<std::type_index, ActionTimePair> actions;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_WORLDITEM_HPP__
