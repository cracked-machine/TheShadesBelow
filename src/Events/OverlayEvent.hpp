#ifndef SRC_EVENTS_OVERLAYEVENT_HPP__
#define SRC_EVENTS_OVERLAYEVENT_HPP__

namespace Game::Events
{

//! @brief Signals that the player has requested an overlay action, e.g. toggling the pause overlay. Handled by
//! Sys::OverlayArbitrationSystem, which applies the modal policy to the toggles, and Sys::GrimoireOverlaySystem
//! (page turning).
struct OverlayEvent
{
  //! @brief The kind of overlay action requested.
  enum class OverlayActions {
    //! @brief Toggle the pause overlay on/off. Ignored while another modal overlay or the debug panels are open,
    //! see Sys::OverlayArbitrationSystem::can_open_modal().
    PAUSE_TOGGLE,
    //! @brief Toggle the grimoire overlay open/closed (debounced). Ignored while another modal overlay or the debug
    //! panels are open, see Sys::OverlayArbitrationSystem::can_open_modal().
    GRIMOIRE_TOGGLE,
    //! @brief Go to the previous grimoire page. No-op if the grimoire is closed.
    GRIMOIRE_PREV,
    //! @brief Go to the next grimoire page. No-op if the grimoire is closed.
    GRIMOIRE_NEXT,
    //! @brief Toggle the F3 debug panels (Cmp::SceneSettings::ShowDebugStats). Ignored while a modal overlay is open.
    DEBUG_TOGGLE,
    //! @brief Move the heal menu selection up a row. Handled by Sys::SpringOverlaySystem / Sys::BarberOverlaySystem.
    HEAL_PREV,
    //! @brief Move the heal menu selection down a row. Handled by Sys::SpringOverlaySystem / Sys::BarberOverlaySystem.
    HEAL_NEXT,
    //! @brief Pay for and apply the selected heal. Handled by Sys::SpringOverlaySystem / Sys::BarberOverlaySystem.
    HEAL_PURCHASE
  };

  //! @brief Construct a new OverlayEvent object
  //! @param act The action requested.
  explicit OverlayEvent( OverlayActions act )
      : action( act )
  {
  }

  //! @brief The action requested.
  OverlayActions action;
};

} // namespace Game::Events

#endif // SRC_EVENTS_OVERLAYEVENT_HPP__
