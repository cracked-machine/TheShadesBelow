#ifndef SRC_EVENTS_GRIMOIRENPCUPDATEEVENT_HPP__
#define SRC_EVENTS_GRIMOIRENPCUPDATEEVENT_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace Game::Events
{

//! @brief Notification that an NPC's action has been applied to the player stats, so the action can be
//! discovered on the NPC's grimoire page.
//! @note Sent only by Sys::NpcSystem, after the action has actually been applied (e.g. not for a skipped spawn).
//! Repeating actions (e.g. proximity ticks) send this on every application.
struct GrimoireNpcUpdateEvent
{
  //! @brief Construct a new GrimoireNpcUpdateEvent object
  //! @param npc NPC identifier, e.g. "npc.adder". See res/json/npc.json.
  //! @param action_key Action identifier, e.g. Cmp::SpawnAction::kKey.
  //! @param action The action that was applied.
  GrimoireNpcUpdateEvent( Sys::NpcKey npc, std::string_view action_key, Cmp::BaseAction action )
      : m_npc( std::move( npc ) ),
        m_action_key( action_key ),
        m_action( std::move( action ) )
  {
  }

  //! @brief NPC identifier of the NPC whose action was applied.
  Sys::NpcKey m_npc;
  //! @brief Action identifier, e.g. "spawn_action".
  std::string m_action_key;
  //! @brief The action that was applied.
  Cmp::BaseAction m_action;
};

} // namespace Game::Events

#endif // SRC_EVENTS_GRIMOIRENPCUPDATEEVENT_HPP__
