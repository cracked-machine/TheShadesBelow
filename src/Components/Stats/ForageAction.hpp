#ifndef SRC_COMPONENTS_STATS_FORAGEACTION_HPP__
#define SRC_COMPONENTS_STATS_FORAGEACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <optional>
#include <string_view>
#include <utility>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied when the player forages the plant holding this action.
//! @note Optional per item. When foraging with gloves the stat changes are skipped but the side effects still apply.
class ForageAction : public BaseAction
{
public:
  //! @brief Key identifying this action in res/json/items.json and res/json/npc.json.
  static constexpr std::string_view kKey = "forage_action";

  //! @brief Construct a new Forage Action object.
  //! @param health Change applied to the health stat.
  //! @param fear Change applied to the fear stat.
  //! @param despair Change applied to the despair stat.
  //! @param infamy Change applied to the infamy stat.
  //! @param tick How often (seconds) the action re-applies, or 0 for a one-shot.
  //! @param toxidrome Toxidrome affliction applied alongside the stat changes, if any.
  //! @param side_effects Optional side effects (NPC spawn, lightning, movement).
  //! @param reward Optional reward granted when the action is applied (see Cmp::ActionReward).
  ForageAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck, Stats::Tick tick,
                Cmp::Toxicity::Toxidrome toxidrome = {}, SideEffects side_effects = {}, std::optional<ActionReward> reward = {} )
      : BaseAction( health, fear, despair, infamy, luck, tick, std::move( toxidrome ), std::move( side_effects ), std::move( reward ) )
  {
  }
  //! @brief Destroy the Forage Action object.
  ~ForageAction() {}

private:
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_FORAGEACTION_HPP__
