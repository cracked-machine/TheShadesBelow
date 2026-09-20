#ifndef SRC_COMPONENTS_STATS_SACRIFICEACTION_HPP__
#define SRC_COMPONENTS_STATS_SACRIFICEACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <utility>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied when an item is sacrificed at an altar.
class SacrificeAction : public BaseAction
{
public:
  //! @brief Construct a new Sacrifice Action object.
  //! @param health Change applied to the health stat.
  //! @param fear Change applied to the fear stat.
  //! @param despair Change applied to the despair stat.
  //! @param infamy Change applied to the infamy stat.
  //! @param toxicity Change applied to the toxicity stat.
  //! @param tick How often (seconds) the action re-applies, or 0 for a one-shot.
  //! @param toxidrome Toxidrome affliction applied alongside the stat changes, if any.
  SacrificeAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Toxicity toxicity, Stats::Luck luck,
                   Stats::Tick tick, Cmp::Toxicity::Toxidrome toxidrome = {} )
      : BaseAction( health, fear, despair, infamy, toxicity, luck, tick, std::move( toxidrome ) )
  {
  }
  //! @brief Destroy the Sacrifice Action object.
  ~SacrificeAction() {}

private:
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_SACRIFICEACTION_HPP__
