#ifndef SRC_COMPONENTS_STATS_PROXIMITYACTION_HPP__
#define SRC_COMPONENTS_STATS_PROXIMITYACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <string_view>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied repeatedly, at its own tick interval, while the player is near
//! (and the triggering entity is within the current screen view of) the entity holding this action.
class ProximityAction : public BaseAction
{
public:
  //! @brief Key identifying this action in res/json/items.json and res/json/npc.json.
  static constexpr std::string_view kKey = "proximity_action";

  //! @brief The same constructors as BaseAction.
  using BaseAction::BaseAction;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_PROXIMITYACTION_HPP__
