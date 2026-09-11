#ifndef SRC_CMPS_ALTAR_ALTARACTIVATIONTIMER_HPP__
#define SRC_CMPS_ALTAR_ALTARACTIVATIONTIMER_HPP__

#include <SFML/System/Time.hpp>

namespace Game::Cmp::Altar
{

//! @brief Added to the player entity when altar activation cooldown is in effect.
class ActivationTimer : public sf::Time
{
public:
  sf::Time get_timeout() { return m_timeout; };

private:
  sf::Time m_timeout{ sf::seconds( 3.f ) };
};

} // namespace Game::Cmp::Altar

#endif // SRC_CMPS_ALTAR_ALTARACTIVATIONTIMER_HPP__
