#ifndef SRC_CMPS_PLAYER_BURNINGTIMER_HPP__
#define SRC_CMPS_PLAYER_BURNINGTIMER_HPP__

#include <SFML/System/Time.hpp>
namespace Game::Cmp::Plant
{

//! @brief
class BurningTimer : public sf::Time
{
public:
  sf::Time timeout() { return m_timeout; }

private:
  sf::Time m_timeout{ sf::seconds( 9.0f ) };
};

} // namespace Game::Cmp::Plant

#endif // SRC_CMPS_PLAYER_BURNINGTIMER_HPP__