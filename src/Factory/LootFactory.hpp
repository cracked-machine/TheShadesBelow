#ifndef SRC_FACTORY_LOOTFACTORY_HPP__
#define SRC_FACTORY_LOOTFACTORY_HPP__

#include <Components/AnimData.hpp>
#include <Components/Loot.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Position.hpp>
#include <Components/ZOrderValue.hpp>
#include <Factory/Factory.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/BaseSystem.hpp>

#include <entt/entity/registry.hpp>

namespace Game::Sprites
{
class SpriteFactory;

} // namespace Game::Sprites

namespace Game::Factory::Loot
{

//! @brief Turn `entt` into a loot container entity (chest/pot etc.) that can later drop loot.
//! @param registry
//! @param entt Entity to attach the loot container components to.
//! @param pos_cmp
//! @param sprite_type
//! @param sprite_tile_idx
//! @param zorder
void create_loot_container( entt::registry &registry, entt::entity entt, Cmp::Position pos_cmp, Sprites::SpriteMetaType sprite_type,
                            std::size_t sprite_tile_idx, float zorder );

//! @brief Remove the components added by create_loot_container(), without destroying the entity.
//! @param registry
//! @param loot_entity
//! @param reserved_sm If provided, un-reserves the container's position.
void destroy_loot_container( entt::registry &registry, entt::entity loot_entity,
                             const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr );

namespace detail
{
//! @brief Whether any entity with component `T` occupies `pos`.
//! @tparam T Component type to search for.
//! @param registry
//! @param pos
//! @return true if a `T`-tagged entity overlaps `pos`.
template <typename T>
bool has_component_at_pos( entt::registry &registry, const Cmp::Position &pos )
{
  for ( auto [entt, cmp, cmp_pos] : registry.view<T, Cmp::Position>().each() )
  {
    if ( pos.findIntersection( cmp_pos ) ) return true;
  }
  return false;
}

//! @brief Whether `pos` is occupied by any entity carrying one of the `SpatialExclude` component types.
//! @tparam SpatialExclude Component types to check spatially (separate entities at the same position).
//! @param registry
//! @param pos
//! @return true if any excluded component is found at `pos`.
template <typename... SpatialExclude>
bool has_any_spatial_excluded_at_pos( entt::registry &registry, const Cmp::Position &pos, ExcludePack<SpatialExclude...> )
{
  return ( has_component_at_pos<SpatialExclude>( registry, pos ) || ... );
}
} // namespace detail

//! @brief Attempts to create a loot drop at a traversable position within a specified search area.
//! @tparam Include Component types the position entity must have.
//! @tparam Exclude Component types the position entity must NOT have.
//! @tparam SpatialExclude Component types to check spatially (separate entities at same position).
//! @param registry The entity registry.
//! @param loot_anim_cmp The sprite animation for the loot.
//! @param search The area in which to search for a suitable position.
//! @param zorder_offset Offset added to position.y for z-ordering.
//! @param reserved_sm If provided, skips positions already reserved (O(1) check).
//! @return The newly created loot entity, or entt::null if no suitable location was found.
template <typename... Include, typename... Exclude, typename... SpatialExclude>
inline entt::entity create_loot_drop( entt::registry &registry, Cmp::AnimData &&loot_anim_cmp, sf::FloatRect search, IncludePack<Include...>,
                                      ExcludePack<Exclude...>, ExcludePack<SpatialExclude...> spatial_exclude = ExcludePack<>{},
                                      float zorder_offset = -8.f, const PathFinding::SpatialHashGrid *reserved_sm = nullptr )
{
  auto pos_view = registry.view<Cmp::Position, Include...>( entt::exclude<Exclude...> );

  for ( auto [pos_entity, pos_cmp] : pos_view.each() )
  {
    if ( not search.findIntersection( pos_cmp ) ) continue;

    // Skip non-traversable obstacles
    auto obst_cmp = registry.try_get<Cmp::Obstacle>( pos_entity );

    // Skip positions occupied by spatially excluded entities
    if ( detail::has_any_spatial_excluded_at_pos( registry, pos_cmp, spatial_exclude ) ) continue;

    // Skip positions reserved from algorithmic changes
    if ( ( reserved_sm != nullptr ) && not reserved_sm->at( pos_cmp ).empty() ) continue;

    auto new_loot_entity = registry.create();
    registry.emplace<Cmp::Position>( new_loot_entity, pos_cmp.position, pos_cmp.size );
    registry.emplace<Cmp::AnimData>( new_loot_entity, loot_anim_cmp );
    registry.emplace<Cmp::ZOrderValue>( new_loot_entity, pos_cmp.position.y + zorder_offset );
    registry.emplace<Cmp::Loot>( new_loot_entity );
    SPDLOG_DEBUG( "Created loot entity {} of type {} at position ({}, {})", static_cast<int>( new_loot_entity ), loot_anim_cmp.m_sprite_type,
                  pos_cmp.position.x, pos_cmp.position.y );
    return new_loot_entity;
  }

  SPDLOG_WARN( "Failed to drop {} at [{},{}].", loot_anim_cmp.m_sprite_type, search.position.x, search.position.y );
  return entt::null;
}

//! @brief Destroy a loot drop entity created by create_loot_drop().
//! @param registry
//! @param loot_entity
void destroy_loot_drop( entt::registry &registry, entt::entity loot_entity );

} // namespace Game::Factory::Loot

#endif // SRC_FACTORY_LOOTFACTORY_HPP__
