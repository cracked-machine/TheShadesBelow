#ifndef SRC_COMPONENTS_STATS_ACTIONSET_HPP__
#define SRC_COMPONENTS_STATS_ACTIONSET_HPP__

#include <Components/Stats/BaseActionFwd.hpp>

#include <memory>
#include <typeindex>

namespace Game::Cmp
{

//! @brief The actions of a Cmp::WorldItem or Cmp::Npc::NPC, keyed by the std::type_index of the concrete action type
//! (e.g. Cmp::CarryAction). The map lives behind a pointer so that WorldItem.hpp and Npc.hpp only need
//! BaseActionFwd.hpp: the many files that use those components without touching actions don't rebuild when
//! BaseAction.hpp changes. Copies are deep.
//! @note Every action is optional in res/json/items.json and res/json/npc.json: a missing one reads as no_action().
class ActionSet
{
public:
  ActionSet();
  ~ActionSet();
  ActionSet( const ActionSet &other );
  ActionSet &operator=( const ActionSet &other );
  ActionSet( ActionSet &&other ) noexcept;
  ActionSet &operator=( ActionSet &&other ) noexcept;

  //! @brief Get the action/time pair for key. If absent, a zeroed (no effect) action is inserted, so the returned
  //! timer is owned by this set. Use contains() beforehand if "absent" must be told apart from "present but no effect".
  [[nodiscard]] ActionTimePair &at( std::type_index key );

  //! @brief Get the action/time pair for key, or a shared zeroed (no effect) pair if absent.
  [[nodiscard]] const ActionTimePair &at( std::type_index key ) const;

  //! @brief Whether an action is stored for key.
  [[nodiscard]] bool contains( std::type_index key ) const;

  //! @brief Add an action for key. Does nothing if key already has one.
  void emplace( std::type_index key, const BaseAction &action );

  //! @brief All stored actions.
  [[nodiscard]] const ActionMap &map() const;

private:
  //! @brief The stored actions. Null only in a moved-from set, which reads as empty.
  std::unique_ptr<ActionMap> m_map;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_ACTIONSET_HPP__
