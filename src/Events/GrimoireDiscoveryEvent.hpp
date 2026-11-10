#ifndef SRC_EVENTS_GRIMOIREDISCOVERYEVENT_HPP__
#define SRC_EVENTS_GRIMOIREDISCOVERYEVENT_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <string>
#include <utility>

namespace Game::Events
{

//! @brief Notification that the grimoire revealed new information about an item or NPC: its page entry or a newly
//! discovered action.
//! @note Sent by Sys::GrimoireSystem only when the grimoire actually changed, unlike Events::GrimoireItemUpdateEvent /
//! Events::GrimoireNpcUpdateEvent
//! which is sent on every application of an action. See Sys::TooltipSystem.
struct GrimoireDiscoveryEvent
{
  //! @brief Construct a new GrimoireDiscoveryEvent object for an item page
  //! @param item Item identifier, e.g. "item.plant.nightshade". See res/json/items.json.
  explicit GrimoireDiscoveryEvent( const Sys::ItemKey &item )
      : m_key( item.str() )
  {
  }

  //! @brief Construct a new GrimoireDiscoveryEvent object for an NPC page
  //! @param npc NPC identifier, e.g. "npc.adder". See res/json/npc.json.
  explicit GrimoireDiscoveryEvent( const Sys::NpcKey &npc )
      : m_key( npc.str() )
  {
  }

  //! @brief Identifier of the item ("item.*") or NPC ("npc.*") whose grimoire entry changed.
  std::string m_key;
};

} // namespace Game::Events

#endif // SRC_EVENTS_GRIMOIREDISCOVERYEVENT_HPP__
