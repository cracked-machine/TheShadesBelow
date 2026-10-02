#ifndef SRC_EVENTS_CREATEITEMEVENT_HPP__
#define SRC_EVENTS_CREATEITEMEVENT_HPP__

#include <Components/Position.hpp>
#include <Systems/Stores/StoreKey.hpp>
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
  explicit CreateItemEvent( Cmp::Position pos, Sys::ItemKey item, std::string sfx = "", float zorder = 0.f )
      : m_pos( pos ),
        m_item( std::move( item ) ),
        m_sfx( std::move( sfx ) ),
        m_zorder( zorder )
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
};

} // namespace Game::Events

#endif // SRC_EVENTS_CREATEITEMEVENT_HPP__
