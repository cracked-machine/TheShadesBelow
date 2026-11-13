#ifndef SRC_COMPONENTS_STATS_COLLISIONACTION_HPP__
#define SRC_COMPONENTS_STATS_COLLISIONACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <string_view>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied when the player physically collides with an NPC.
class CollisionAction : public BaseAction
{
public:
  //! @brief Key identifying this action in res/json/items.json and res/json/npc.json.
  static constexpr std::string_view kKey = "collision_action";

  //! @brief The same constructors as BaseAction.
  using BaseAction::BaseAction;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_COLLISIONACTION_HPP__
