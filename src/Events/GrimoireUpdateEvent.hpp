#ifndef SRC_EVENTS_GRIMOIREUPDATEEVENT_HPP__
#define SRC_EVENTS_GRIMOIREUPDATEEVENT_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace Game::Events
{

//! @brief Notification that an item's action has been applied to the player stats, so the action can be
//! discovered in the grimoire.
//! @note Sent by the Utils::Player::apply_action_from_* helpers after the stats have been updated.
//! Repeating actions (e.g. burn/proximity ticks) send this on every application.
struct GrimoireUpdateEvent
{
  //! @brief Construct a new GrimoireUpdateEvent object
  //! @param item Item identifier, e.g. "item.plant.nightshade". See res/json/items.json.
  //! @param action_key Action identifier, e.g. Cmp::BurnAction::kKey.
  //! @param action The action that was applied.
  GrimoireUpdateEvent( Sys::ItemKey item, std::string_view action_key, Cmp::BaseAction action )
      : m_item( std::move( item ) ),
        m_action_key( action_key ),
        m_action( std::move( action ) )
  {
  }

  //! @brief Item identifier of the item whose action was applied.
  Sys::ItemKey m_item;
  //! @brief Action identifier, e.g. "burn_action".
  std::string m_action_key;
  //! @brief The action that was applied.
  Cmp::BaseAction m_action;
};

} // namespace Game::Events

#endif // SRC_EVENTS_GRIMOIREUPDATEEVENT_HPP__
