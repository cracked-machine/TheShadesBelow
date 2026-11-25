#ifndef SRC_EVENTS_LEAVEREFUSEDEVENT_HPP__
#define SRC_EVENTS_LEAVEREFUSEDEVENT_HPP__

#include <SFML/Graphics/Rect.hpp>

namespace Game::Events
{

//! @brief Notification that the horse refused to leave the graveyard because every cart barrel is empty.
//! @note Sent by Sys::CartHorseSystem. See Sys::TooltipOverlaySystem.
struct LeaveRefusedEvent
{
  //! @brief World bounds of the horse, e.g. to place a tooltip above it
  sf::FloatRect horse_bounds;
};

} // namespace Game::Events

#endif // SRC_EVENTS_LEAVEREFUSEDEVENT_HPP__
