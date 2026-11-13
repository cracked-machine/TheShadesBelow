#ifndef SRC_COMPONENTS_STATS_PROJECTILEACTION_HPP__
#define SRC_COMPONENTS_STATS_PROJECTILEACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <string_view>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied when the player is hit by a ranged projectile (e.g. a priest's
//! projectile or a watchman's gunfire).
class ProjectileAction : public BaseAction
{
public:
  //! @brief Key identifying this action in res/json/items.json and res/json/npc.json.
  static constexpr std::string_view kKey = "projectile_action";

  //! @brief The same constructors as BaseAction.
  using BaseAction::BaseAction;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_PROJECTILEACTION_HPP__
