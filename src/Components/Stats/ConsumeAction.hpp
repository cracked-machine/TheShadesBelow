#ifndef SRC_COMPONENTS_STATS_CONSUMEACTION_HPP__
#define SRC_COMPONENTS_STATS_CONSUMEACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <string_view>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied when an item is consumed (eaten/drunk/used up).
class ConsumeAction : public BaseAction
{
public:
  //! @brief Key identifying this action in res/json/items.json and res/json/npc.json.
  static constexpr std::string_view kKey = "consume_action";

  //! @brief The same constructors as BaseAction.
  using BaseAction::BaseAction;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_CONSUMEACTION_HPP__
