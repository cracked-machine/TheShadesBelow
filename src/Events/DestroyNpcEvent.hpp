#ifndef SRC_EVENTS_DESTROYNPCEVENT_HPP__
#define SRC_EVENTS_DESTROYNPCEVENT_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <entt/entity/entity.hpp>

#include <optional>
#include <utility>

namespace Game::Events
{

//! @brief Kill an NPC, optionally playing a death animation where it stood.
//! @note Consumed by NpcSystem, which removes the NPC from its navmesh and strips its movement and type
//! components. Trigger this instead of calling Factory::Npc::destroy_npc directly: a direct call leaves a
//! stale navmesh entry behind, along with the NPC's Cmp::LerpPosition and type marker components.
class DestroyNpcEvent
{
public:
  //! @param npc_entity The NPC to kill.
  //! @param death_anim Sprite to play at the NPC's position, or std::nullopt for no animation.
  explicit DestroyNpcEvent( entt::entity npc_entity, std::optional<Sys::SpriteKey> death_anim = std::nullopt )
      : m_npc_entity( npc_entity ),
        m_death_anim( std::move( death_anim ) )
  {
  }
  entt::entity m_npc_entity;
  std::optional<Sys::SpriteKey> m_death_anim;
};

} // namespace Game::Events

#endif // SRC_EVENTS_DESTROYNPCEVENT_HPP__
