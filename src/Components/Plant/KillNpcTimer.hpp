#ifndef SRC_CMPS_PLANT_KILLNPCTIMER_HPP__
#define SRC_CMPS_PLANT_KILLNPCTIMER_HPP__

#include <SFML/System/Time.hpp>
namespace Game::Cmp::Plant
{

//! @brief Time since a plant's proximity_action kill_npc effect last rolled, see Sys::NpcSystem::check_plant_kill_npc.
//! @note Kept apart from the proximity_action's own timer, which Sys::PlayerSystem resets whenever the player is near.
class KillNpcTimer : public sf::Time
{
};

} // namespace Game::Cmp::Plant

#endif // SRC_CMPS_PLANT_KILLNPCTIMER_HPP__
