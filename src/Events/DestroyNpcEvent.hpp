#ifndef SRC_EVENTS_DESTROYNPCEVENT_HPP__
#define SRC_EVENTS_DESTROYNPCEVENT_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <entt/entity/entity.hpp>

#include <optional>
#include <utility>

namespace Game::Events
{

//! @brief Kill an NPC, optionally playing a death animation where it stood. This removes the entity!
//! @note Consumed by NpcSystem, which removes the NPC from its navmesh and strips its movement and type
//! components. Trigger this instead of calling Factory::Npc::destroy_npc directly: a direct call leaves a
//! stale navmesh entry behind, along with the NPC's Cmp::LerpPosition and type marker components.
class DestroyNpcEvent
{
public:
  //! @brief What killed the NPC. Some NPCs only die to a particular cause, e.g. the Burryman to the axe.
  enum class Cause {
    //! @brief Anything other than the player's axe or arrows, e.g. a bomb, fire or banishing.
    OTHER,
    //! @brief The player's axe.
    AXE,
    //! @brief An arrow fired by the player.
    ARROW
  };

  //! @param npc_entity The NPC to kill.
  //! @param death_anim Sprite to play at the NPC's position, or std::nullopt for no animation.
  //! @param cause What killed the NPC.
  explicit DestroyNpcEvent( entt::entity npc_entity, std::optional<Sys::SpriteKey> death_anim = std::nullopt, Cause cause = Cause::OTHER )
      : m_npc_entity( npc_entity ),
        m_death_anim( std::move( death_anim ) ),
        m_cause( cause )
  {
  }
  entt::entity m_npc_entity;
  std::optional<Sys::SpriteKey> m_death_anim;
  Cause m_cause;
};

} // namespace Game::Events

#endif // SRC_EVENTS_DESTROYNPCEVENT_HPP__
