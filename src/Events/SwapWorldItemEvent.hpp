#ifndef SRC_EVENTS_SWAPWORLDITEMEVENT_HPP__
#define SRC_EVENTS_SWAPWORLDITEMEVENT_HPP__

#include <Systems/Stores/StoreKey.hpp>
#include <entt/entity/fwd.hpp>

#include <string>
#include <utility>

namespace Game::Events
{

//! @brief Requests that an existing world item entity be replaced in place with another item type,
//! e.g. a worn out lightning rod becoming a broken lightning rod.
//! @note Consumed by ItemSystem.
struct SwapWorldItemEvent
{
  //! @brief Construct a new SwapWorldItemEvent object
  //! @param entt The world item entity to swap.
  //! @param new_item Item identifier of the replacement item, e.g. "item.lightningrod.broken".
  //! @param sfx Sound effect key to play when the item is swapped; empty for no sound.
  explicit SwapWorldItemEvent( entt::entity entt, Sys::ItemKey new_item, std::string sfx = "" )
      : m_entt( entt ),
        m_new_item( std::move( new_item ) ),
        m_sfx( std::move( sfx ) )
  {
  }

  //! @brief The world item entity to swap.
  entt::entity m_entt;
  //! @brief Item identifier of the replacement item.
  Sys::ItemKey m_new_item;
  //! @brief Sound effect key to play when the item is swapped; empty for no sound.
  std::string m_sfx;
};

} // namespace Game::Events

#endif // SRC_EVENTS_SWAPWORLDITEMEVENT_HPP__
