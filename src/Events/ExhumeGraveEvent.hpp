#ifndef SRC_EVENTS_EXHUMEGRAVEEVENT_HPP__
#define SRC_EVENTS_EXHUMEGRAVEEVENT_HPP__

#include <entt/entity/entity.hpp>

namespace Game::Events
{

//! @brief Requests that a grave be exhumed by something other than the player digging, e.g. a bomb blast.
//! @note Consumed by Sys::GraveSystem, which opens the grave and applies its consequence. Loot and keys are
//! dropped beside the grave rather than at/into the player.
struct ExhumeGraveEvent
{
  //! @brief Construct a new ExhumeGraveEvent object
  //! @param grave_entt The grave's Cmp::Grave::MultiBlock entity.
  explicit ExhumeGraveEvent( entt::entity grave_entt )
      : m_grave_entt( grave_entt )
  {
  }

  //! @brief The grave's Cmp::Grave::MultiBlock entity.
  entt::entity m_grave_entt;
};

} // namespace Game::Events

#endif // SRC_EVENTS_EXHUMEGRAVEEVENT_HPP__
