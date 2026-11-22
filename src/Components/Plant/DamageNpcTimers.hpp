#ifndef SRC_CMPS_PLANT_DAMAGENPCTIMERS_HPP__
#define SRC_CMPS_PLANT_DAMAGENPCTIMERS_HPP__

#include <SFML/System/Time.hpp>

#include <vector>

namespace Game::Cmp::Plant
{

//! @brief Time since each entry of a plant's proximity_action damage_npc effect last dealt damage, see
//! Sys::NpcSystem::check_plant_damage_npc. One timer per Cmp::DamageNpcEffect::entries element, at the same index.
//! @note Kept apart from the proximity_action's own timer, which Sys::PlayerSystem resets whenever the player is near.
struct DamageNpcTimers
{
  std::vector<sf::Time> timers;
};

} // namespace Game::Cmp::Plant

#endif // SRC_CMPS_PLANT_DAMAGENPCTIMERS_HPP__
