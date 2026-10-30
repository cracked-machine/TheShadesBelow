#ifndef SRC_COMPONENTS_STATS_DESTROYACTION_HPP__
#define SRC_COMPONENTS_STATS_DESTROYACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <optional>
#include <string_view>
#include <utility>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied when an entity (e.g. an NPC) is destroyed/removed, applied to
//! the player before the destroyed entity's components are removed.
class DestroyAction : public BaseAction
{
public:
  //! @brief Key identifying this action in res/json/items.json and res/json/npc.json.
  static constexpr std::string_view kKey = "destroy_action";

  //! @brief Construct a new Destroy Action object.
  //! @param health Change applied to the health stat.
  //! @param fear Change applied to the fear stat.
  //! @param despair Change applied to the despair stat.
  //! @param infamy Change applied to the infamy stat.
  //! @param tick How often (seconds) the action re-applies, or 0 for a one-shot.
  //! @param toxidrome Disease affliction applied alongside the stat changes, if any.
  //! @param side_effects Optional side effects (NPC spawn, lightning, movement).
  //! @param rewards Optional rewards granted when the action is applied (see Cmp::ActionReward).
  DestroyAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck, Stats::Tick tick,
                 Cmp::Toxicity::Toxidrome toxidrome = {}, SideEffects side_effects = {}, ActionRewards rewards = {} )
      : BaseAction( health, fear, despair, infamy, luck, tick, std::move( toxidrome ), std::move( side_effects ), std::move( rewards ) )
  {
  }
  //! @brief Destroy the Destroy Action object.
  ~DestroyAction() {}

private:
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_DESTROYACTION_HPP__
