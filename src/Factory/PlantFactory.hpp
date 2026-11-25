#ifndef SRC_FACTORY_PLANTFACTORY_HPP__
#define SRC_FACTORY_PLANTFACTORY_HPP__

#include <SFML/Graphics/Rect.hpp>
#include <entt/entity/fwd.hpp>

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

//! @brief Remove the grass (Cmp::Plant::Grass) overlapping `area` via Events::DestroyPlantEvent, e.g. when a hazard
//! cell or a replanted plant takes over its cell. Grass doesn't reserve its tile, so nothing else stops them overlapping.
//! @param reg
//! @param area The area being taken over, in pixels.
void destroy_grass_in( entt::registry &reg, const sf::FloatRect &area );

} // namespace Game::Factory::Plant

#endif // SRC_FACTORY_PLANTFACTORY_HPP__
