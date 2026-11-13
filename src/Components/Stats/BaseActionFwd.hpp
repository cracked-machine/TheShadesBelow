#ifndef SRC_COMPONENTS_STATS_BASEACTIONFWD_HPP__
#define SRC_COMPONENTS_STATS_BASEACTIONFWD_HPP__

//! @file
//! @brief Forward declarations for Components/Stats/BaseAction.hpp. Include this from headers that only pass actions
//! around by reference or declare functions returning them, so they don't rebuild whenever BaseAction.hpp changes.

#include <vector>

namespace Game::Cmp
{

class BaseAction;
struct ActionReward;
struct ActionWear;
struct SpawnNpcEffect;
struct ActionTimePair;
struct ActionMap;

//! @brief The rewards of an action. Only an offering_action may have more than one, each for a different altar location.
using ActionRewards = std::vector<ActionReward>;

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_BASEACTIONFWD_HPP__
