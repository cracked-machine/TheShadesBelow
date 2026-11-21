#ifndef SRC_EVENTS_CROWGUIDEEVENT_HPP__
#define SRC_EVENTS_CROWGUIDEEVENT_HPP__

#include <Components/Position.hpp>

#include <entt/entity/entity.hpp>

namespace Game::Events
{

//! @brief Send a guide crow from the activated altar to lead the player to the key grave. The crow lands at the edge of
//! the view towards the grave and, each time the player approaches, flies on to the next edge point, until the grave is
//! in view; then it perches on the grave until it is exhumed.
//! @note Sent by Sys::GraveSystem, consumed by Sys::NpcSystem.
struct CrowGuideEvent
{
  //! @brief The grave tagged with Cmp::Grave::KeyGrave
  entt::entity key_grave{ entt::null };
  //! @brief The activated altar's bounds; the crow takes off from its centre
  Cmp::Position altar_position;
};

} // namespace Game::Events

#endif // SRC_EVENTS_CROWGUIDEEVENT_HPP__
