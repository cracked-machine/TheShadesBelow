#ifndef SRC_CMP_SCENESETTINGS_PAUSED_HPP__
#define SRC_CMP_SCENESETTINGS_PAUSED_HPP__

namespace Game::Cmp::SceneSettings
{

//! @brief Scene-level setting for the pause overlay, toggled with the P key. While enabled the scene keeps drawing the
//! world but stops advancing it, and Sys::PauseOverlaySystem draws the pause overlay on top.
struct Paused
{
  //! @brief Whether the game is paused.
  bool enabled{ false };
};

} // namespace Game::Cmp::SceneSettings

#endif // SRC_CMP_SCENESETTINGS_PAUSED_HPP__
