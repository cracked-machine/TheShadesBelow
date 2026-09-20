#ifndef SRC_COMPONENTS_STATS_COLLISIONACTION_HPP__
#define SRC_COMPONENTS_STATS_COLLISIONACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <utility>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied when the player physically collides with an NPC.
class CollisionAction : public BaseAction
{
public:
  //! @brief Construct a new Collision Action object.
  //! @param health Change applied to the health stat.
  //! @param fear Change applied to the fear stat.
  //! @param despair Change applied to the despair stat.
  //! @param infamy Change applied to the infamy stat.
  //! @param tick How often (seconds) the action re-applies, or 0 for a one-shot.
  //! @param disease Disease affliction applied alongside the stat changes, if any.
  CollisionAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck,
                   Stats::Tick tick, Cmp::Toxicity::Toxidrome toxidrome = {} )
      : BaseAction( health, fear, despair, infamy, luck, tick, std::move( toxidrome ) )
  {
  }
  //! @brief Destroy the Collision Action object.
  ~CollisionAction() {}

private:
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_COLLISIONACTION_HPP__
