#ifndef SRC_EVENTS_CREATEPLANTEVENT_HPP__
#define SRC_EVENTS_CREATEPLANTEVENT_HPP__

#include <Components/Position.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <entt/entity/entity.hpp>

#include <functional>
#include <utility>

namespace Game::Events
{

//! @brief Create a plant multiblock at a world position.
//! @note Consumed by PlantSystem, which keeps the reserved spatial map in sync. Trigger this instead of creating
//! plant multiblocks directly.
struct CreatePlantEvent
{
  //! @brief Construct a new CreatePlantEvent object
  //! @param plant_type The plant world item, e.g. "item.plant.nettle". See res/json/items.json.
  //! @param position The top-left of the plant's footprint.
  //! @param on_created Optional callback receiving the new plant entity. Not called if the footprint isn't clear.
  CreatePlantEvent( Sys::ItemKey plant_type, Cmp::Position position, std::function<void( entt::entity )> on_created = {} )
      : m_plant_type( std::move( plant_type ) ),
        m_position( std::move( position ) ),
        m_on_created( std::move( on_created ) )
  {
  }

  //! @brief The plant world item, see res/json/items.json.
  Sys::ItemKey m_plant_type;
  //! @brief The top-left of the plant's footprint.
  Cmp::Position m_position;
  //! @brief Optional callback receiving the new plant entity.
  std::function<void( entt::entity )> m_on_created;
};

} // namespace Game::Events

#endif // SRC_EVENTS_CREATEPLANTEVENT_HPP__
