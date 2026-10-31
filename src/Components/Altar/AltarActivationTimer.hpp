#ifndef SRC_CMPS_ALTAR_ALTARACTIVATIONTIMER_HPP__
#define SRC_CMPS_ALTAR_ALTARACTIVATIONTIMER_HPP__

#include <SFML/System/Time.hpp>

namespace Game::Cmp::Altar
{

//! @brief Added to the player entity when altar activation cooldown is in effect: the time since the last offering.
//! The cooldown itself is Cmp::Persist::AltarOfferingCooldown, or Cmp::Persist::AltarSummonOfferingCooldown.
class ActivationTimer : public sf::Time
{
};

} // namespace Game::Cmp::Altar

#endif // SRC_CMPS_ALTAR_ALTARACTIVATIONTIMER_HPP__
