#ifndef SRC_EVENTS_CRYPTENTRYREFUSEDEVENT_HPP__
#define SRC_EVENTS_CRYPTENTRYREFUSEDEVENT_HPP__

namespace Game::Events
{

//! @brief Notification that a locked crypt door was not unlocked because the player has already collected every
//! cadaver part, so the crypt has nothing left to give.
//! @note Sent by Sys::CryptSystem. See Sys::TooltipOverlaySystem.
struct CryptEntryRefusedEvent
{
};

} // namespace Game::Events

#endif // SRC_EVENTS_CRYPTENTRYREFUSEDEVENT_HPP__
