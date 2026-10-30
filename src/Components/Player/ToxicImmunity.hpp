#ifndef SRC_COMPONENTS_PLAYER_TOXICIMMUNITY_HPP__
#define SRC_COMPONENTS_PLAYER_TOXICIMMUNITY_HPP__

#include <Components/Player/TimedEffect.hpp>

namespace Game::Cmp::Player
{

//! @brief  While present on the player entity, toxidromes can't increase (existing toxicity stays, cures still apply),
//!         and the toxidrome UI meters are drawn "frozen". Added by a Cmp::ActionReward::Special::TOXIC_IMMUNITY
//!         sacrifice (e.g. St John's Wort) and removed by PlayerSystem once m_timer reaches m_timeout.
class ToxicImmunity : public TimedEffect
{
public:
  using TimedEffect::TimedEffect;
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_TOXICIMMUNITY_HPP__
