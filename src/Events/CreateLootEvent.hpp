#ifndef SRC_EVENTS_CREATELOOTEVENT_HPP__
#define SRC_EVENTS_CREATELOOTEVENT_HPP__

#include <Components/Position.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <entt/entity/entity.hpp>

#include <utility>

namespace Game::Events
{

//! @brief Drop loot on a random free cell beside a source, e.g. gold around a plant.
//! @note Consumed by LootSystem.
struct CreateLootEvent
{
  //! @brief Construct a new CreateLootEvent object
  //! @param loot_type The loot sprite to drop, e.g. sprite.crypt.loot.gold.
  //! @param source The source's bounds; the loot drops on a random free cell from the nearest ring around it.
  //! @param max_instances Max drops over `source_entt`'s lifetime, picked up or not; the drop is skipped once reached. 0 = unlimited.
  //! @param source_entt The dropping world item, or entt::null if there is none (then unlimited).
  CreateLootEvent( Sys::SpriteKey loot_type, Cmp::Position source, int max_instances = 0, entt::entity source_entt = entt::null )
      : m_loot_type( std::move( loot_type ) ),
        m_source( std::move( source ) ),
        m_max_instances( max_instances ),
        m_source_entt( source_entt )
  {
  }

  //! @brief The loot sprite to drop.
  Sys::SpriteKey m_loot_type;
  //! @brief The source's bounds.
  Cmp::Position m_source;
  //! @brief Max drops over m_source_entt's lifetime. 0 = unlimited.
  int m_max_instances{ 0 };
  //! @brief The dropping world item, or entt::null if there is none.
  entt::entity m_source_entt{ entt::null };
};

} // namespace Game::Events

#endif // SRC_EVENTS_CREATELOOTEVENT_HPP__
