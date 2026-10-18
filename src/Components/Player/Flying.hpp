#ifndef SRC_COMPONENTS_PLAYER_FLYING_HPP__
#define SRC_COMPONENTS_PLAYER_FLYING_HPP__

namespace Game::Cmp::Player
{

//! @brief Mark the player as airborne on the witchesbroom (from take-off until touchdown). See Sys::FlyingSystem.
struct Flying
{
  //! @brief True while descending to the landing cell. Sys::FlyingSystem owns the player position until touchdown.
  bool landing{ false };
  //! @brief The sprite has risen above 1x1 obstacles, so it is drawn in front of them. Maintained by Sys::FlyingSystem.
  bool elevated{ false };
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_FLYING_HPP__
