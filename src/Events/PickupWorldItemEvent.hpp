#ifndef SRC_EVENTS_PICKUPWORLDITEMEVENT_HPP__
#define SRC_EVENTS_PICKUPWORLDITEMEVENT_HPP__

namespace Game::Events
{

//! @brief Requests that a world item entity be picked up into a new player inventory slot.
struct PickupWorldItemEvent
{
  //! @brief The world item entity to pick up.
  entt::entity world_item_entt;
  //! @brief true: pick it up into the first empty slot, or leave it on the ground if every slot is full (see
  //! Events::CreateItemEvent::m_player_reward and Cmp::WorldItem::is_auto_pickup()). false: into the selected slot, dropping anything already in it.
  bool to_empty_slot{ false };
};

} // namespace Game::Events

#endif // SRC_EVENTS_PICKUPWORLDITEMEVENT_HPP__
