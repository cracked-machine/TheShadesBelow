#ifndef SRC_EVENTS_GRAVETRAILCOMPLETEEVENT_HPP__
#define SRC_EVENTS_GRAVETRAILCOMPLETEEVENT_HPP__

namespace Game::Events
{

//! @brief Notification that the grave trail has reached its end and the crypt key has been exhumed.
//! @note Sent by Sys::GraveSystem. Sys::AltarSystem responds by resetting the activated altar (candles out, talismans
//! cleared, Cmp::Altar::Activated removed), so any altar, including that one, can be activated with talismans again.
struct GraveTrailCompleteEvent
{
};

} // namespace Game::Events

#endif // SRC_EVENTS_GRAVETRAILCOMPLETEEVENT_HPP__
