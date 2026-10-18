#ifndef SRC_COMPONENTS_PLAYER_CRASHLANDING_HPP__
#define SRC_COMPONENTS_PLAYER_CRASHLANDING_HPP__

namespace Game::Cmp::Player
{

//! @brief Marks a crash landing: the witchesbroom wore out while the player was flying over an obstacle, so they
//! are being snapped to the nearest free cell. Present until touchdown. No behaviour of its own - a hook for
//! future sprite replacement work. See Sys::FlyingSystem.
struct CrashLanding
{
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_CRASHLANDING_HPP__
