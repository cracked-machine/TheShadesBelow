#ifndef SRC_EVENTS_BLOODREFUSEDEVENT_HPP__
#define SRC_EVENTS_BLOODREFUSEDEVENT_HPP__

#include <SFML/Graphics/Rect.hpp>

namespace Game::Events
{

//! @brief Notification that an altar refused the player's blood because an altar is already activated
//! (Cmp::Altar::Activated), so its grave trail must be completed before any altar takes blood again.
//! @note Sent by Sys::AltarSystem. See Sys::TooltipOverlaySystem.
struct BloodRefusedEvent
{
  //! @brief World bounds of the refusing altar multiblock, e.g. to place a tooltip above it
  sf::FloatRect altar_bounds;
};

} // namespace Game::Events

#endif // SRC_EVENTS_BLOODREFUSEDEVENT_HPP__
