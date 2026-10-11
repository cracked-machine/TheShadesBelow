#ifndef SRC_EVENTS_UPDATENPCPOSITIONEVENT_HPP__
#define SRC_EVENTS_UPDATENPCPOSITIONEVENT_HPP__

#include <SFML/System/Vector2.hpp>

#include <entt/entity/entity.hpp>

namespace Game::Events
{

//! @brief Relocate an NPC to a new world position, abandoning any step it is part-way through.
//! @note Consumed by NpcSystem, which keeps the NPC's navmesh in sync with the move. Trigger this instead of
//! modifying the NPC's Cmp::Position directly: a direct write leaves a stale navmesh entry behind, and is
//! overwritten by the NPC's Cmp::LerpPosition if it is mid-step.
class UpdateNpcPositionEvent
{
public:
  //! @param npc_entity The NPC to move.
  //! @param position The NPC's new top-left world position.
  UpdateNpcPositionEvent( entt::entity npc_entity, sf::Vector2f position )
      : m_npc_entity( npc_entity ),
        m_position( position )
  {
  }
  entt::entity m_npc_entity;
  sf::Vector2f m_position;
};

} // namespace Game::Events

#endif // SRC_EVENTS_UPDATENPCPOSITIONEVENT_HPP__
