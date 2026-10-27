#ifndef SRC_FACTORY_MULTIBLOCKFACTORY_HPP__
#define SRC_FACTORY_MULTIBLOCKFACTORY_HPP__

#include <Components/Position.hpp>
#include <Components/UUID.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <concepts>
#include <cstddef>
#include <utility>
#include <vector>
#include <entt/entity/fwd.hpp>
#include <entt/fwd.hpp>

namespace Game::PathFinding
{
class SpatialHashGrid;
}

// Multiblock/segment components are forward declared so that editing one of them does not recompile every
// consumer of this header. Callers include the concrete component headers they actually use.
namespace Game::Cmp
{
class PlantMultiBlock;
class PlantSegment;
class HealingSpringMultiBlock;
class HealingSpringSegment;
class HealingSpringBuildingMultiBlock;
class HealingSpringBuildingSegment;
} // namespace Game::Cmp

namespace Game::Cmp::Altar
{
class MultiBlock;
class Segment;
} // namespace Game::Cmp::Altar

namespace Game::Cmp::Crypt
{
class BuildingMultiBlock;
class BuildingSegment;
class ObjectiveMultiBlock;
class ObjectiveSegment;
} // namespace Game::Cmp::Crypt

namespace Game::Cmp::Grave
{
class MultiBlock;
class Segment;
class ExitMultiBlock;
class ExitSegment;
} // namespace Game::Cmp::Grave

namespace Game::Cmp::Ruin
{
class BuildingMultiBlock;
class BuildingSegment;
class HexagramMultiBlock;
class HexagramSegment;
class StairsLowerMultiBlock;
class StairsUpperMultiBlock;
class StairsBalustradeMultiBlock;
class StairsGateMultiBlock;
class StairsSegment;
class GateSegment;
} // namespace Game::Cmp::Ruin

namespace Game::Sprites
{
class SpriteSheet;
}

namespace Game::Factory::Multiblock
{

//! @brief Component requirements for a multiblock segment type (one grid cell of a multiblock structure).
template <typename T>
concept IsMBSegment = requires( T t ) {
  { t.isCollisionMask() } -> std::convertible_to<bool>;
  { t.set_collision_mask( true ) } -> std::same_as<void>;
};

//! @brief Component requirements for a multiblock bounds type - must derive from sf::FloatRect.
template <typename T>
concept IsMB = std::derived_from<T, sf::FloatRect>;

namespace detail
{

//! @brief Create the multiblock "root" entity: emplaces the MULTIBLOCK bounds, anim, z-order, UUID and position.
//! @tparam MULTIBLOCK Multiblock bounds component type.
//! @param reg
//! @param entity Entity to emplace the multiblock components onto.
//! @param uuid Identifier shared with the multiblock's segment entities.
//! @param pos
//! @param ss
//! @param ss_index Sprite frame index within `ss`.
template <typename MULTIBLOCK>
  requires IsMB<MULTIBLOCK>
void create_multiblock( entt::registry &reg, entt::entity entity, const Cmp::UUID &uuid, Cmp::Position pos, const Sprites::SpriteSheet &ss,
                        size_t ss_index = 0 );

//! @brief Recompute collision mask, z-order and pathfinding-blocking components for every MBSEGMENT inside `mb_cmp`'s bounds.
//! @tparam MULTIBLOCK Multiblock bounds component type.
//! @tparam MBSEGMENT Multiblock segment component type.
//! @param reg
//! @param ss Sprite sheet providing the collision mask and z-order lookup tables.
//! @param mb_entt
//! @param mb_cmp Multiblock bounds used to find owned segments and their relative grid position.
template <typename MULTIBLOCK, typename MBSEGMENT>
  requires IsMB<MULTIBLOCK> && IsMBSegment<MBSEGMENT>
void update_segments( entt::registry &reg, const Sprites::SpriteSheet &ss, entt::entity mb_entt, MULTIBLOCK mb_cmp );

//! @brief Create one MBSEGMENT entity per world tile inside the multiblock's bounds, tagging the door tile
//! with the type-appropriate entrance/exit component.
//! @tparam MULTIBLOCK Multiblock bounds component type.
//! @tparam MBSEGMENT Multiblock segment component type.
//! @param reg
//! @param multiblock_entity
//! @param uuid Identifier shared with the multiblock root entity.
//! @param mb_pos_cmp
//! @param ss Sprite sheet providing the door position and grid size.
//! @param reserved_sm If provided, skips positions already reserved and reserves each new segment's position.
//! @return The newly created segment entities.
template <typename MULTIBLOCK, typename MBSEGMENT>
  requires IsMB<MULTIBLOCK> && IsMBSegment<MBSEGMENT>
std::vector<entt::entity> create_multiblock_segments( entt::registry &reg, entt::entity multiblock_entity, const Cmp::UUID &uuid,
                                                      Cmp::Position mb_pos_cmp, const Sprites::SpriteSheet &ss,
                                                      PathFinding::SpatialHashGrid *reserved_sm = nullptr );

} // namespace detail

//! @brief Create a multiblock structure (e.g. an altar, crypt building, staircase) along with all of its segments.
//! @tparam MULTIBLOCK Multiblock bounds component type.
//! @tparam MBSEGMENT Multiblock segment component type.
//! @param reg
//! @param position World position of the multiblock's origin.
//! @param ss
//! @param ss_index Sprite frame index within `ss`.
//! @param zorder Explicit z-order; if 0, falls back to the sprite sheet's json z-order, then to the y-position.
//! @param reserved_sm If provided, skips positions already reserved and reserves the root and each new segment's position.
//! @return The multiblock root entity and its created segment entities.
template <typename MULTIBLOCK, typename MBSEGMENT>
  requires IsMB<MULTIBLOCK> && IsMBSegment<MBSEGMENT>
std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments( entt::registry &reg, sf::Vector2f position,
                                                                                 const Sprites::SpriteSheet &ss, size_t ss_index = 0,
                                                                                 float zorder = 0, PathFinding::SpatialHashGrid *reserved_sm = nullptr );

} // namespace Game::Factory::Multiblock

//! @note Template definitions live in MultiblockFactoryImpl.hpp and are explicitly instantiated in the
//! MultiblockFactory*.cpp files for every MULTIBLOCK/MBSEGMENT combination used across the codebase. Consumers of
//! this header never see the definitions, so no extern template declarations are needed here.

#endif // SRC_FACTORY_MULTIBLOCKFACTORY_HPP__
