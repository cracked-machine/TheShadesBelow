#ifndef SRC_EVENTS_ARTEFACTREFUSEDEVENT_HPP__
#define SRC_EVENTS_ARTEFACTREFUSEDEVENT_HPP__

#include <SFML/Graphics/Rect.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <utility>

namespace Game::Events
{

//! @brief Notification that an altar refused an artefact because an altar is already activated (Cmp::Altar::Activated),
//! so its grave trail must be completed before any altar takes artefacts again.
//! @note Sent by Sys::AltarSystem. See Sys::TooltipOverlaySystem.
struct ArtefactRefusedEvent
{
  //! @brief Construct a new ArtefactRefusedEvent object
  //! @param item Item identifier of the refused artefact, e.g. "item.artefact.treasure". See res/json/items.json.
  //! @param altar_bounds World bounds of the refusing altar multiblock
  ArtefactRefusedEvent( Sys::ItemKey item, sf::FloatRect altar_bounds )
      : m_item( std::move( item ) ),
        m_altar_bounds( altar_bounds )
  {
  }

  //! @brief Item identifier of the refused artefact.
  Sys::ItemKey m_item;
  //! @brief World bounds of the refusing altar multiblock, e.g. to place a tooltip above it
  sf::FloatRect m_altar_bounds;
};

} // namespace Game::Events

#endif // SRC_EVENTS_ARTEFACTREFUSEDEVENT_HPP__
