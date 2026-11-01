#ifndef SRC_FACTORY_PLANTFACTORY_HPP__
#define SRC_FACTORY_PLANTFACTORY_HPP__

#include <Components/Position.hpp>
#include <entt/entity/fwd.hpp>
#include <Systems/Stores/StoreKey.hpp>

namespace Game::Sprites
{
class SpriteSheet;
} // namespace Game::Sprites

namespace Game::Factory::Plant
{
//! @brief Destroy a plant multiblock entity and its segment entities.
//! @note Does not touch the navmeshes: trigger Events::DestroyPlantEvent instead, whose handler (PlantSystem) keeps
//! them in sync before calling this.
//! @param reg
//! @param plant_entt
void remove_plant_mb( entt::registry &reg, entt::entity plant_entt );

} // namespace Game::Factory::Plant

#endif // SRC_FACTORY_PLANTFACTORY_HPP__
