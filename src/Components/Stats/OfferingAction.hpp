#ifndef SRC_COMPONENTS_STATS_OFFERINGACTION_HPP__
#define SRC_COMPONENTS_STATS_OFFERINGACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <string_view>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied when an item is offered at an altar.
class OfferingAction : public BaseAction
{
public:
  //! @brief Key identifying this action in res/json/items.json and res/json/npc.json.
  static constexpr std::string_view kKey = "offering_action";

  //! @brief The same constructors as BaseAction.
  using BaseAction::BaseAction;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_OFFERINGACTION_HPP__
