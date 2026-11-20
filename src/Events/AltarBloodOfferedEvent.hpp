#ifndef SRC_EVENTS_ALTARBLOODOFFEREDEVENT_HPP__
#define SRC_EVENTS_ALTARBLOODOFFEREDEVENT_HPP__

#include <SFML/Graphics/Rect.hpp>

namespace Game::Events
{

//! @brief Notification that a graveyard altar has just taken the player's health as blood
//! (Cmp::Persist::AltarBloodOfferingHealth).
//! @note Sent by Sys::AltarSystem. See Sys::TooltipOverlaySystem.
struct AltarBloodOfferedEvent
{
  //! @brief World bounds of the altar multiblock, e.g. to place a tooltip above it
  sf::FloatRect altar_bounds;
};

} // namespace Game::Events

#endif // SRC_EVENTS_ALTARBLOODOFFEREDEVENT_HPP__
