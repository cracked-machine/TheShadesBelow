#ifndef SRC_FACTORY_PLANTFACTORY_HPP__
#define SRC_FACTORY_PLANTFACTORY_HPP__

#include <Components/Position.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/SpriteMetaType.hpp>

namespace Game::Sprites
{
class SpriteSheet;
class SpriteFactory;
} // namespace Game::Sprites

namespace Game::Sprites
{
class SpriteFactory;

} // namespace Game::Sprites

namespace Game::Factory::Plant
{
//! @brief Remove the components of plant multiblock and segment entities.
//! Also keeps the scene navmeshes in sync: destroyed segments are removed from the
//! player navmesh and the freed world tiles become walkable in the NPC navmesh.
//! @param reg
//! @param plant_entt
//! @param npc_navmesh optional NPC navmesh to re-insert the freed tiles into
//! @param player_navmesh optional player navmesh to remove the destroyed segments from
//! @param reserved_sm optional reserved-positions navmesh to un-reserve the freed tiles from
void remove_plant_mb( entt::registry &reg, entt::entity plant_entt, const PathFinding::SpatialHashGridSharedPtr &npc_navmesh = nullptr,
                      const PathFinding::SpatialHashGridSharedPtr &player_navmesh = nullptr,
                      const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr );

} // namespace Game::Factory::Plant

#endif // SRC_FACTORY_PLANTFACTORY_HPP__
