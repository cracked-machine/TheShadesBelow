#ifndef SRC_COMPONENTS_PLAYER_EXTRALIFE_HPP__
#define SRC_COMPONENTS_PLAYER_EXTRALIFE_HPP__

#include <Components/UUID.hpp>
#include <SFML/System/Vector2.hpp>

#include <optional>

namespace Game::Cmp::Player
{

//! @brief Marks the player as having an extra life available, preventing death when active.
//! @details Transferred with the player between scenes, so it can be spent in a different scene to the one it was granted in.
struct ExtraLife
{
  //! @brief The altar that granted the extra life.
  std::optional<Cmp::UUID> altar_uuid;
  //! @brief Where the player respawns when the extra life is used, if spent in the scene of the granting altar.
  //! std::nullopt (or spent in another scene) means the current scene's player start position.
  std::optional<sf::Vector2f> respawn_position;
  //! @brief Whether the extra life is currently available/active.
  [[maybe_unused]] bool active{ true };
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_EXTRALIFE_HPP__
