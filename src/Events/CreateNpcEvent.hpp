#ifndef SRC_EVENTS_CREATENPCEVENT_HPP__
#define SRC_EVENTS_CREATENPCEVENT_HPP__

#include <Components/Position.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <entt/entity/entity.hpp>

#include <functional>
#include <utility>

namespace Game::Events
{

//! @brief Spawn an NPC at a world position.
//! @note Consumed by NpcSystem, which keeps the NPC's navmesh in sync. Trigger this instead of calling
//! Factory::Npc::create_npc() directly.
struct CreateNpcEvent
{
  //! @brief Construct a new CreateNpcEvent object
  //! @param npc_type See res/json/npc.json.
  //! @param position The spawn position. The size is replaced by the NPC sprite's size.
  //! @param unique If true and NPCs of `npc_type` already exist, relocate them to `position` instead of spawning another.
  //! @param on_created Optional callback receiving the newly created NPC entity. Not called when relocating or if creation fails.
  CreateNpcEvent( Sys::NpcKey npc_type, Cmp::Position position, bool unique = false, std::function<void( entt::entity )> on_created = {} )
      : m_npc_type( std::move( npc_type ) ),
        m_position( std::move( position ) ),
        m_unique( unique ),
        m_on_created( std::move( on_created ) )
  {
  }

  //! @brief The NPC to spawn, see res/json/npc.json.
  Sys::NpcKey m_npc_type;
  //! @brief The spawn position.
  Cmp::Position m_position;
  //! @brief Relocate existing NPCs of this type instead of spawning another.
  bool m_unique{ false };
  //! @brief Optional callback receiving the newly created NPC entity.
  std::function<void( entt::entity )> m_on_created;
};

} // namespace Game::Events

#endif // SRC_EVENTS_CREATENPCEVENT_HPP__
