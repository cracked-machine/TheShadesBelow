#ifndef SRC_COMPONENTS_STATS_FORAGEACTION_HPP__
#define SRC_COMPONENTS_STATS_FORAGEACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <string_view>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied when the player forages the plant holding this action.
//! @note Optional per item. When foraging with gloves the stat changes are skipped but the side effects still apply.
class ForageAction : public BaseAction
{
public:
  //! @brief Key identifying this action in res/json/items.json and res/json/npc.json.
  static constexpr std::string_view kKey = "forage_action";

  //! @brief The same constructors as BaseAction.
  using BaseAction::BaseAction;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_FORAGEACTION_HPP__
