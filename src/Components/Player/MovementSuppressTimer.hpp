#ifndef SRC_CMPS_PLAYER_MOVEMENTSUPPRESSTIMER_HPP__
#define SRC_CMPS_PLAYER_MOVEMENTSUPPRESSTIMER_HPP__

#include <SFML/System/Time.hpp>

namespace Game::Cmp::Player
{

//! @brief Added/refreshed on the player entity after a push/pull obstacle move to briefly suppress
//!        further player movement, animation and push/pull checks.
struct MovementSuppressTimer : public sf::Time
{
};

} // namespace Game::Cmp::Player

#endif // SRC_CMPS_PLAYER_MOVEMENTSUPPRESSTIMER_HPP__
