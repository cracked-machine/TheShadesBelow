#ifndef SRC_EVENTS_ALTARBLOODOFFEREDEVENT_HPP__
#define SRC_EVENTS_ALTARBLOODOFFEREDEVENT_HPP__

#include <SFML/Graphics/Rect.hpp>

namespace Game::Events
{

//! @brief Notification that the player has just poured a blood vial into a graveyard altar
//! (see Sys::AltarSystem::offering_blood_vial()).
//! @note Sent by Sys::AltarSystem. See Sys::TooltipOverlaySystem.
struct AltarBloodOfferedEvent
{
  //! @brief World bounds of the altar multiblock, e.g. to place a tooltip above it
  sf::FloatRect altar_bounds;
};

} // namespace Game::Events

#endif // SRC_EVENTS_ALTARBLOODOFFEREDEVENT_HPP__
