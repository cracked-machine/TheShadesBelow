#ifndef SRC_COMPONENTS_PLAYER_CAMERAHOLD_HPP__
#define SRC_COMPONENTS_PLAYER_CAMERAHOLD_HPP__

#include <SFML/System/Time.hpp>

namespace Game::Cmp::Player
{

//! @brief While present on the player entity, the world camera stays where it is instead of following the player,
//!        e.g. so a teleport's departure animation stays in view.
//! @note Added and ticked down by TeleportSystem; read by RenderGameSystem::update_camera.
struct CameraHold
{
  //! @brief Time left before the camera is released. TeleportSystem removes the component once this reaches zero.
  sf::Time remaining{ sf::Time::Zero };
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_CAMERAHOLD_HPP__
