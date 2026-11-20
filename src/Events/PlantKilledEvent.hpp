#ifndef SRC_EVENTS_PLANTKILLEDEVENT_HPP__
#define SRC_EVENTS_PLANTKILLEDEVENT_HPP__

#include <entt/entity/entity.hpp>

namespace Game::Events
{

//! @brief A player weapon hit (shovel or arrow) took a plant's Cmp::Health to zero. PlantSystem applies the plant's
//! destroy_action: drops its reward, triggers its side effects and removes it.
struct PlantKilledEvent
{
  //! @brief Construct a new PlantKilledEvent object
  //! @param plant_entity The Cmp::PlantMultiBlock entity that was killed.
  explicit PlantKilledEvent( entt::entity plant_entity )
      : m_plant_entity( plant_entity )
  {
  }

  //! @brief The Cmp::PlantMultiBlock entity that was killed.
  entt::entity m_plant_entity;
};

} // namespace Game::Events

#endif // SRC_EVENTS_PLANTKILLEDEVENT_HPP__
