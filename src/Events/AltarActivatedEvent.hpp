#ifndef SRC_EVENTS_ALTARACTIVATEDEVENT_HPP__
#define SRC_EVENTS_ALTARACTIVATEDEVENT_HPP__

#include <Components/Position.hpp>

namespace Game::Events
{

//! @brief Triggered by AltarSystem once the player pays anima to activate an altar with every artefact
//! offered, and it becomes Cmp::Altar::Activated.
//! GraveSystem responds by choosing the key grave trail start graves.
struct AltarActivatedEvent
{
  //! @brief The activated altar's bounds
  Cmp::Position altar_position;
};

} // namespace Game::Events

#endif // SRC_EVENTS_ALTARACTIVATEDEVENT_HPP__
