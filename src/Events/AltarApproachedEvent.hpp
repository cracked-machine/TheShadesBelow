#ifndef SRC_EVENTS_ALTARAPPROACHEDEVENT_HPP__
#define SRC_EVENTS_ALTARAPPROACHEDEVENT_HPP__

#include <SFML/Graphics/Rect.hpp>

namespace Game::Events
{

//! @brief Notification that the player has just come near a graveyard altar. Sent once per approach, not every frame
//! while the player stays near.
//! @note Sent by Sys::AltarSystem. See Sys::TooltipOverlaySystem.
struct AltarApproachedEvent
{
  //! @brief World bounds of the altar multiblock, e.g. to place a tooltip above it
  sf::FloatRect altar_bounds;
};

} // namespace Game::Events

#endif // SRC_EVENTS_ALTARAPPROACHEDEVENT_HPP__
