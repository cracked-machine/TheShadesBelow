#ifndef SRC_EVENTS_DISMISSTOOLTIPEVENT_HPP__
#define SRC_EVENTS_DISMISSTOOLTIPEVENT_HPP__

namespace Game::Events
{

//! @brief Requests that the currently displayed tooltip be hidden before its timer expires.
//! See Sys::TooltipSystem.
struct DismissTooltipEvent
{
};

} // namespace Game::Events

#endif // SRC_EVENTS_DISMISSTOOLTIPEVENT_HPP__
