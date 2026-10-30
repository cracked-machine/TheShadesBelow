#ifndef SRC_EVENTS_TELEPORTENTITYEVENT_HPP__
#define SRC_EVENTS_TELEPORTENTITYEVENT_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entity/entity.hpp>

#include <optional>
#include <string>
#include <utility>

namespace Game::Events
{

//! @brief Teleport an actor (player or NPC) to a new world position, optionally playing an animation as it goes.
//! @note Consumed by TeleportSystem, which forwards the move to Events::UpdatePlayerPositionEvent or
//! Events::UpdateNpcPositionEvent so the navmeshes stay in sync. Trigger this instead of writing Cmp::Position directly.
class TeleportEntityEvent
{
public:
  //! @param entity The player or NPC to teleport.
  //! @param position The entity's new top-left world position.
  //! @param anim Sprite to play at both the departure and arrival positions, or std::nullopt for no animation.
  //! @param sfx Sound effect to play when the player teleports, or std::nullopt for none.
  //! @param delay Non-zero fades the entity out over this time (Cmp::PendingTeleport) before it is moved.
  TeleportEntityEvent( entt::entity entity, sf::Vector2f position, std::optional<Sys::SpriteKey> anim = std::nullopt,
                       std::optional<std::string> sfx = std::nullopt, sf::Time delay = sf::Time::Zero )
      : m_entity( entity ),
        m_position( position ),
        m_anim( std::move( anim ) ),
        m_sfx( std::move( sfx ) ),
        m_delay( delay )
  {
  }
  entt::entity m_entity;
  sf::Vector2f m_position;
  std::optional<Sys::SpriteKey> m_anim;
  std::optional<std::string> m_sfx;
  sf::Time m_delay;
};

} // namespace Game::Events

#endif // SRC_EVENTS_TELEPORTENTITYEVENT_HPP__
