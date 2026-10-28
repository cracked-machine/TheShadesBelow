#ifndef SRC_EVENTS_CREATEITEMEVENT_HPP__
#define SRC_EVENTS_CREATEITEMEVENT_HPP__

#include <Components/Position.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <entt/entity/entity.hpp>
#include <utility>

namespace Game::Events
{

//! @brief Requests that a world item entity be spawned, e.g. dropped loot or a level-generated item.
struct CreateItemEvent
{

  //! @brief Construct a new CreateItemEvent object
  //! @param pos World position at which to spawn the item.
  //! @param item Sprite meta type name of the item to create.
  //! @param sfx Sound effect key to play when the item is created; empty for no sound.
  //! @param zorder Draw order for the spawned item sprite.
  //! @param spawn_nearby true: `pos` is the source's bounds; spawn on a random free cell from the nearest ring around it.
  //! @param max_instances Max drops over `source_entt`'s lifetime (with `spawn_nearby`), picked up or not; the drop is skipped once reached.
  //! 0 = unlimited.
  //! @param source_entt The dropping world item, or entt::null if there is none (then unlimited).
  explicit CreateItemEvent( Cmp::Position pos, Sys::ItemKey item, std::string sfx = "", float zorder = 0.f, bool spawn_nearby = false,
                            int max_instances = 0, entt::entity source_entt = entt::null )
      : m_pos( pos ),
        m_item( std::move( item ) ),
        m_sfx( std::move( sfx ) ),
        m_zorder( zorder ),
        m_spawn_nearby( spawn_nearby ),
        m_max_instances( max_instances ),
        m_source_entt( source_entt )
  {
  }

  //! @brief World position at which to spawn the item.
  Cmp::Position m_pos;
  //! @brief Sprite meta type name of the item to create.
  Sys::ItemKey m_item;
  //! @brief Sound effect key to play when the item is created; empty for no sound.
  std::string m_sfx;
  //! @brief Draw order for the spawned item sprite.
  float m_zorder;
  //! @brief true: m_pos is the source's bounds; spawn on a random free cell from the nearest ring around it.
  bool m_spawn_nearby{ false };
  //! @brief Max drops over m_source_entt's lifetime. 0 = unlimited.
  int m_max_instances{ 0 };
  //! @brief The dropping world item, or entt::null if there is none.
  entt::entity m_source_entt{ entt::null };
};

} // namespace Game::Events

#endif // SRC_EVENTS_CREATEITEMEVENT_HPP__
