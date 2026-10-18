#ifndef SRC_COMPONENTS_PLAYER_SOLIDBOUNDARY_HPP__
#define SRC_COMPONENTS_PLAYER_SOLIDBOUNDARY_HPP__

namespace Game::Cmp::Player
{

//! @brief Mark a world position made solid by the level's "solid" object layer (see Factory::Wall::add_solid_player).
//! These shape the level boundary so, like walls, they still block the player while flying. See Sys::FlyingSystem::blocks_flight.
struct SolidBoundary
{
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_SOLIDBOUNDARY_HPP__
