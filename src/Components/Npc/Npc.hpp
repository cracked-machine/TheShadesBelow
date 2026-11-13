#ifndef SRC_COMPONENTS_NPC_NPC_HPP__
#define SRC_COMPONENTS_NPC_NPC_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <typeindex>

namespace Game::Cmp::Npc
{

//! @brief ECS component holding an NPC's sprite animation set, lerp parameter,
//! and the currently active gameplay-stat action modifiers with their elapsed times.
class NPC
{
public:
  //! @brief Default constructor. Leaves sprite_type_list empty and the lerp speed at 0.
  NPC() = default;

  //! @brief Construct an NPC with a single sprite type.
  //! @param sprite_type The sprite to use, wrapped into a single-element sprite_type_list.
  //! @param lerp_speed Interpolation speed used when smoothing this NPC's movement.
  NPC( Sys::SpriteKey sprite_type, float lerp_speed )
      : sprite_type_list( std::vector<Sys::SpriteKey>{ std::move( sprite_type ) } ),
        m_lerp_speed( lerp_speed )
  {
  }

  //! @brief Construct an NPC with multiple sprite types (e.g. for animation).
  //! @param sprite_type_list The list of sprites, see res/json/npc.json.
  //! @param lerp_speed Interpolation speed used when smoothing this NPC's movement.
  NPC( std::vector<Sys::SpriteKey> sprite_type_list, float lerp_speed )
      : sprite_type_list( std::move( sprite_type_list ) ),
        m_lerp_speed( lerp_speed )
  {
  }
  // bool active = true;

  //! @brief The NPC's type key in res/json/npc.json, e.g. "npc.eldermother". Set by Sys::NpcStore.
  Sys::NpcKey npc_type;

  //! @brief The associated sprite. Supports multiple sprites for animation. See res/json/npc.json.
  std::vector<Sys::SpriteKey> sprite_type_list;

  //! @brief Interpolation speed used when smoothing this NPC's movement.
  float m_lerp_speed{ 0 };

  //! @brief Object holding a list of action modifiers and time object
  struct ActionTimePair
  {
    //! @brief Record of player stat modifiers for a specific action.
    BaseAction action;
    //! @brief Time elapsed for a specific action. This can be used for applying modifiers from a specific NPC instance.
    sf::Time time;
  };

  //! @brief Get the action/time pair keyed by ActionT. Every action is optional in npc.json: if absent, a zeroed
  //! (no effect) action is inserted, so the returned timer is owned by this NPC. Use has_action() beforehand if
  //! "absent" must be told apart from "present but no effect".
  template <typename ActionT>
  ActionTimePair &at()
  {
    return actions.try_emplace( std::type_index( typeid( ActionT ) ), ActionTimePair{ no_action(), sf::Time::Zero } ).first->second;
  }
  //! @brief Get the action/time pair keyed by ActionT, or a shared zeroed (no effect) pair if absent.
  template <typename ActionT>
  const ActionTimePair &at() const
  {
    static const ActionTimePair kNone{ no_action(), sf::Time::Zero };
    const auto it = actions.find( std::type_index( typeid( ActionT ) ) );
    return it != actions.end() ? it->second : kNone;
  }
  //! @brief Whether the NPC has an ActionT, see res/json/npc.json.
  template <typename ActionT>
  [[nodiscard]] bool has_action() const
  {
    return actions.contains( std::type_index( typeid( ActionT ) ) );
  }
  template <typename ActionT>
  void emplace( ActionT action )
  {
    actions.emplace( std::type_index( typeid( ActionT ) ), ActionTimePair{ action, sf::Time::Zero } );
  }
  auto begin() { return actions.begin(); }
  auto end() { return actions.end(); }

private:
  //! @brief The list of action modifiers and their current elapsed time
  std::unordered_map<std::type_index, ActionTimePair> actions;
};
} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_NPC_HPP__
