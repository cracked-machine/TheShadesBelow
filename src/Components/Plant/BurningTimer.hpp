#ifndef SRC_CMPS_PLAYER_BURNINGTIMER_HPP__
#define SRC_CMPS_PLAYER_BURNINGTIMER_HPP__

#include <SFML/System/Time.hpp>
namespace Game::Cmp::Plant
{

//! @brief Time a plant has been burning for. The burn duration comes from Cmp::Persist::PlantBurnTime.
class BurningTimer : public sf::Time
{
};

} // namespace Game::Cmp::Plant

#endif // SRC_CMPS_PLAYER_BURNINGTIMER_HPP__
