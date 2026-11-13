#ifndef SRC_COMPONENTS_STATS_DESTROYACTION_HPP__
#define SRC_COMPONENTS_STATS_DESTROYACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <string_view>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied when an entity (e.g. an NPC) is destroyed/removed, applied to
//! the player before the destroyed entity's components are removed.
class DestroyAction : public BaseAction
{
public:
  //! @brief Key identifying this action in res/json/items.json and res/json/npc.json.
  static constexpr std::string_view kKey = "destroy_action";

  //! @brief The same constructors as BaseAction.
  using BaseAction::BaseAction;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_DESTROYACTION_HPP__
