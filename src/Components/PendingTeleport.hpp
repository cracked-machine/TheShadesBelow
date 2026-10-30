#ifndef SRC_COMPONENTS_PENDINGTELEPORT_HPP__
#define SRC_COMPONENTS_PENDINGTELEPORT_HPP__

#include <Events/TeleportEntityEvent.hpp>

#include <SFML/System/Time.hpp>

#include <optional>

namespace Game::Cmp
{

//! @brief Countdown to a teleport. While present, TeleportSystem fades the actor out via Cmp::AbsoluteAlpha.
//! @details Added by TeleportSystem for a delayed Events::TeleportEntityEvent, or by WormholeSystem when an actor
//!          collides with a wormhole. TeleportSystem ticks `elapsed` for every instance.
struct PendingTeleport
{
  //! @brief Gameplay time counting up towards `duration`. Accumulated by TeleportSystem.
  sf::Time elapsed{};
  //! @brief Length of the fade out / countdown before the actor is teleported.
  sf::Time duration{ sf::seconds( 2.0f ) };
  //! @brief Move that TeleportSystem performs once `elapsed` reaches `duration`. std::nullopt when the owner
  //!        (WormholeSystem) picks the destination and triggers the move itself.
  std::optional<Events::TeleportEntityEvent> event{};
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_PENDINGTELEPORT_HPP__
