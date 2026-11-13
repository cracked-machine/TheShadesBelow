#ifndef SRC_COMPONENTS_STATS_SPAWNACTION_HPP__
#define SRC_COMPONENTS_STATS_SPAWNACTION_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <string_view>

namespace Game::Cmp
{

//! @brief Player-stat modifier applied when an entity spawns (e.g. loot dropped by a destroyed grave
//! or NPC).
class SpawnAction : public BaseAction
{
public:
  //! @brief Key identifying this action in res/json/items.json and res/json/npc.json.
  static constexpr std::string_view kKey = "spawn_action";

  //! @brief The same constructors as BaseAction.
  using BaseAction::BaseAction;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_SPAWNACTION_HPP__
