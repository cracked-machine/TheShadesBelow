#ifndef SRC_FACTORY_BOMBFACTORY_HPP__
#define SRC_FACTORY_BOMBFACTORY_HPP__

#include <Components/Armed.hpp>
#include <SFML/System/Time.hpp>
#include <entt/fwd.hpp>

// clang-format off
namespace Game::Cmp { class Position; }
// clang-format on

namespace Game::Factory::Bomb
{

//! @brief Arm `entity` with a Cmp::Armed component, staggering its fuse/warning delays by `sequence`.
//! @param registry
//! @param entity Entity to arm. Reused as the armed entity - no new entity is created.
//! @param epi_center Whether this entity is the blast epicenter; if so it also gets the bomb sprite/z-order.
//! @param sequence Chain position, used to stagger the fuse and warning delays across linked armed entities.
//! @param zorder Z-order applied when `epi_center` is EpiCenter::YES.
//! @return `entity`, unchanged.
entt::entity create_armed( entt::registry &registry, entt::entity entity, Cmp::Armed::EpiCenter epi_center, int sequence, float zorder );

//! @brief Remove the Armed/AnimData/ZOrderValue/NoPathFinding components added by create_armed().
//! @param reg
//! @param armed_entity
void destroy_armed( entt::registry &reg, entt::entity armed_entity );

//! @brief Transition an armed entity into its post-detonation state (obstacle destroyed, no longer blocking).
//! @param reg
//! @param armed_entity
//! @param armed_pos_cmp Position used to compute the detonated sprite's z-order.
void add_detonated( entt::registry &reg, entt::entity armed_entity, Cmp::Position &armed_pos_cmp );

} // namespace Game::Factory::Bomb

#endif // SRC_FACTORY_BOMBFACTORY_HPP__
