#ifndef SRC_EVENTS_CREATENPCEVENT_HPP__
#define SRC_EVENTS_CREATENPCEVENT_HPP__

#include <Components/Position.hpp>
#include <Components/UUID.hpp>
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
  //! @param max_instances Max live NPCs spawned by `source_uuid`; the spawn is skipped once reached. 0 = unlimited.
  //! @param source_uuid The Cmp::UUID of the spawning world item, or empty if there is none (then unlimited).
  //! @param on_created Optional callback receiving the newly created NPC entity. Not called when skipped or if creation fails.
  //! @param spawn_nearby true: `position` is the source's bounds; spawn on a random free cell from the nearest ring around it.
  CreateNpcEvent( Sys::NpcKey npc_type, Cmp::Position position, int max_instances = 0, Cmp::UUID source_uuid = {},
                  std::function<void( entt::entity )> on_created = {}, bool spawn_nearby = false )
      : m_npc_type( std::move( npc_type ) ),
        m_position( std::move( position ) ),
        m_max_instances( max_instances ),
        m_source_uuid( source_uuid ),
        m_on_created( std::move( on_created ) ),
        m_spawn_nearby( spawn_nearby )
  {
  }

  //! @brief The NPC to spawn, see res/json/npc.json.
  Sys::NpcKey m_npc_type;
  //! @brief The spawn position.
  Cmp::Position m_position;
  //! @brief Max live NPCs spawned by m_source_uuid. 0 = unlimited.
  int m_max_instances{ 0 };
  //! @brief The Cmp::UUID of the spawning world item, or empty if there is none.
  Cmp::UUID m_source_uuid;
  //! @brief Optional callback receiving the newly created NPC entity.
  std::function<void( entt::entity )> m_on_created;
  //! @brief true: m_position is the source's bounds; spawn on a random free cell from the nearest ring around it.
  bool m_spawn_nearby{ false };
};

} // namespace Game::Events

#endif // SRC_EVENTS_CREATENPCEVENT_HPP__
