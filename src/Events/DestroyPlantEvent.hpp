#ifndef SRC_EVENTS_DESTROYPLANTEVENT_HPP__
#define SRC_EVENTS_DESTROYPLANTEVENT_HPP__

#include <entt/entity/entity.hpp>

namespace Game::Events
{

//! @brief Remove a plant multiblock and its segments, e.g. when it is depleted, chopped down, dug up or burnt out.
//! @note Consumed by PlantSystem, which keeps the navmeshes in sync. Trigger this instead of calling
//! Factory::Plant::remove_plant_mb directly.
struct DestroyPlantEvent
{
  //! @brief Construct a new DestroyPlantEvent object
  //! @param plant_entity The Cmp::PlantMultiBlock entity to remove.
  explicit DestroyPlantEvent( entt::entity plant_entity )
      : m_plant_entity( plant_entity )
  {
  }

  //! @brief The Cmp::PlantMultiBlock entity to remove.
  entt::entity m_plant_entity;
};

} // namespace Game::Events

#endif // SRC_EVENTS_DESTROYPLANTEVENT_HPP__
