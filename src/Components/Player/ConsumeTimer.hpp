#ifndef SRC_CMPS_PLAYER_EATINGTIMER_HPP__
#define SRC_CMPS_PLAYER_EATINGTIMER_HPP__

#include <SFML/System/Time.hpp>
namespace Game::Cmp::Player
{

//! @brief
class ConsumeTimer : public sf::Time
{
public:
  sf::Time timeout() { return m_timeout; }

private:
  sf::Time m_timeout{ sf::seconds( 3.f ) };
};

} // namespace Game::Cmp::Player

#endif // SRC_CMPS_PLAYER_EATINGTIMER_HPP__