#ifndef SRC_COMPONENTS_STATS_BURNACTION_HPP__
#define SRC_COMPONENTS_STATS_BURNACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <utility>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied repeatedly, at its own tick interval, while an item is held in
//! the player's inventory (e.g. a carried candle accumulating darkness fear over time).
class BurnAction : public BaseAction
{
public:
  //! @brief Construct a new Carry Action object.
  //! @param health Change applied to the health stat.
  //! @param fear Change applied to the fear stat.
  //! @param despair Change applied to the despair stat.
  //! @param infamy Change applied to the infamy stat.
  //! @param tick How often (seconds) the action re-applies, or 0 for a one-shot.
  //! @param disease Disease affliction applied alongside the stat changes, if any.
  BurnAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck,
              Stats::Tick tick, Cmp::Toxicity::Toxidrome toxidrome = {} )
      : BaseAction( health, fear, despair, infamy, luck, tick, std::move( toxidrome ) )
  {
  }
  //! @brief Destroy the Carry Action object.
  ~BurnAction() {}

private:
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_BURNACTION_HPP__
