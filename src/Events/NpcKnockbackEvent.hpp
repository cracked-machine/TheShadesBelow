#ifndef SRC_EVENTS_NPCKNOCKBACKEVENT_HPP__
#define SRC_EVENTS_NPCKNOCKBACKEVENT_HPP__

#include <SFML/System/Vector2.hpp>

#include <entt/entity/entity.hpp>

namespace Game::Events
{

//! @brief Knock an NPC back one cell after the player hits it (melee or arrow) without killing it.
//! @note Consumed by NpcSystem::on_npc_knockback_event(), which picks a free cell and moves the NPC there with an
//! Events::UpdateNpcPositionEvent.
class NpcKnockbackEvent
{
public:
  //! @param npc_entity The NPC that was hit.
  //! @param direction The direction of the hit, from the attacker towards the NPC. Need not be normalized.
  NpcKnockbackEvent( entt::entity npc_entity, sf::Vector2f direction )
      : m_npc_entity( npc_entity ),
        m_direction( direction )
  {
  }
  entt::entity m_npc_entity;
  sf::Vector2f m_direction;
};

} // namespace Game::Events

#endif // SRC_EVENTS_NPCKNOCKBACKEVENT_HPP__
