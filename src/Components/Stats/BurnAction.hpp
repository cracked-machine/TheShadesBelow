#ifndef SRC_COMPONENTS_STATS_BURNACTION_HPP__
#define SRC_COMPONENTS_STATS_BURNACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <string_view>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied repeatedly, at its own tick interval, while an item is held in
//! the player's inventory (e.g. a carried torch accumulating darkness fear over time).
class BurnAction : public BaseAction
{
public:
  //! @brief Key identifying this action in res/json/items.json and res/json/npc.json.
  static constexpr std::string_view kKey = "burn_action";

  //! @brief The same constructors as BaseAction.
  using BaseAction::BaseAction;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_BURNACTION_HPP__
