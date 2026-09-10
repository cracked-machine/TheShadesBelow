#ifndef SRC_CMPS_PLAYER_PLAYERPOST_DEATH_TIMEOUT_HPP__
#define SRC_CMPS_PLAYER_PLAYERPOST_DEATH_TIMEOUT_HPP__

#include <SFML/System/Time.hpp>
namespace Game::Cmp::Player
{

//! @brief Added to the player entity after death occurs.
//!        Used as a cooldown timer to allow death animation to complete.
class PostDeathTimer : public sf::Time
{
public:
  sf::Time get_timeout() { return m_post_death_timeout; };

private:
  sf::Time m_post_death_timeout{ sf::seconds( 5.0f ) };
};

} // namespace Game::Cmp::Player

#endif // SRC_CMPS_PLAYER_PLAYERPOST_DEATH_TIMEOUT_HPP__