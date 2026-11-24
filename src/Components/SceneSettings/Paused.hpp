#ifndef SRC_CMP_SCENESETTINGS_PAUSED_HPP__
#define SRC_CMP_SCENESETTINGS_PAUSED_HPP__

namespace Game::Cmp::SceneSettings
{

//! @brief Scene-level pause setting. While paused, for any reason, the scene keeps drawing the world but stops advancing
//! it. A single reason is held, so the pause menu and a pausing tooltip can never be active together.
struct Paused
{
  //! @brief Why the scene is paused
  enum class Reason {
    //! @brief Not paused
    NONE,
    //! @brief The pause menu is open (P key), see Sys::PauseOverlaySystem
    PAUSEMENU,
    //! @brief A tooltip with "pause": true is shown, see Sys::TooltipOverlaySystem
    TOOLTIP,
    //! @brief The "leave the graveyard?" prompt is open (activating the horse), see Sys::LeaveGraveyardOverlaySystem
    LEAVE_PROMPT
  };

  //! @brief Why the scene is paused, NONE if it is not.
  Reason reason{ Reason::NONE };
};

} // namespace Game::Cmp::SceneSettings

#endif // SRC_CMP_SCENESETTINGS_PAUSED_HPP__
