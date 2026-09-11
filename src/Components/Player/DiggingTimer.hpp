#ifndef SRC_CMPS_PLAYER_DIGGINGTIMER_HPP__
#define SRC_CMPS_PLAYER_DIGGINGTIMER_HPP__

#include <SFML/System/Time.hpp>

namespace Game::Cmp::Player
{

//! @brief Added to the player entity when digging cooldown is in effect.
struct DiggingTimer : public sf::Time
{
};

} // namespace Game::Cmp::Player

#endif // SRC_CMPS_PLAYER_DIGGINGTIMER_HPP__
