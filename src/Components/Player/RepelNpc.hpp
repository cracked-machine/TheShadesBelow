#ifndef SRC_COMPONENTS_PLAYER_REPELNPC_HPP__
#define SRC_COMPONENTS_PLAYER_REPELNPC_HPP__

#include <Components/Player/TimedEffect.hpp>

namespace Game::Cmp::Player
{

//! @brief  While present on the player entity, NPCs flee from the player instead of chasing them, and the player
//!         is stained red. Added by a Cmp::MovementEffect::Npc::Type::REPEL side effect (e.g. drinking blood oil)
//!         and removed by PlayerSystem once m_timer reaches m_timeout.
class RepelNpc : public TimedEffect
{
public:
  using TimedEffect::TimedEffect;
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_REPELNPC_HPP__
