#ifndef SRC_EVENTS_NPCPROXIMITYEVENT_HPP__
#define SRC_EVENTS_NPCPROXIMITYEVENT_HPP__

#include <Components/Position.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <utility>

namespace Game::Events
{

//! @brief An NPC's proximity action is due: the NPC is near the player and its tick has elapsed.
//! @note Consumed by Sys::NpcSystem, which applies the NPC's Cmp::ProximityAction (see res/json/npc.json) to the player and notifies
//! the grimoire. Triggered synchronously, so the caller can check the player's health straight afterwards.
struct NpcProximityEvent
{
  //! @brief Construct a new NpcProximityEvent object
  //! @param npc_type See res/json/npc.json.
  //! @param source The NPC's position, the source of any side effects.
  NpcProximityEvent( Sys::NpcKey npc_type, Cmp::Position source )
      : m_npc_type( std::move( npc_type ) ),
        m_source( std::move( source ) )
  {
  }

  //! @brief The NPC whose action applies, see res/json/npc.json.
  Sys::NpcKey m_npc_type;
  //! @brief The NPC's position, the source of any side effects.
  Cmp::Position m_source;
};

} // namespace Game::Events

#endif // SRC_EVENTS_NPCPROXIMITYEVENT_HPP__
