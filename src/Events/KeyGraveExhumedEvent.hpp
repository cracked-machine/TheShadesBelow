#ifndef SRC_EVENTS_KEYGRAVEEXHUMEDEVENT_HPP__
#define SRC_EVENTS_KEYGRAVEEXHUMEDEVENT_HPP__

namespace Game::Events
{

//! @brief Notification that the key grave (Cmp::Grave::KeyGrave) has been exhumed.
//! @note Sent by Sys::GraveSystem. Sys::AltarSystem responds by resetting the activated altar (lights out, blood
//! emptied, Cmp::Altar::Activated removed), so any altar, including that one, takes blood again.
struct KeyGraveExhumedEvent
{
};

} // namespace Game::Events

#endif // SRC_EVENTS_KEYGRAVEEXHUMEDEVENT_HPP__
