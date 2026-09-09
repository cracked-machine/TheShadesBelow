#ifndef SRC_FACTORY_NPCFACTORY_HPP__
#define SRC_FACTORY_NPCFACTORY_HPP__

#include <Components/Position.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/SpriteMetaType.hpp>
#include <entt/fwd.hpp>

namespace Game::Sprites
{
class SpriteSheet;
class SpriteFactory;
} // namespace Game::Sprites

namespace Game::Sprites
{
class SpriteFactory;

} // namespace Game::Sprites

namespace Game::Factory::Npc
{

//! @brief Turn `entt` into an NPC spawn container (e.g. bones pile), used to lazily spawn a real NPC later.
//! @param registry
//! @param entt Entity to attach the container components to.
//! @param pos_cmp
//! @param sprite_type
//! @param sprite_tile_idx
//! @param zorder
void create_npc_container( entt::registry &registry, entt::entity entt, Cmp::Position pos_cmp, Sprites::SpriteMetaType sprite_type,
                           std::size_t sprite_tile_idx, float zorder );

//! @brief Remove the components added by create_npc_container(), without destroying the entity.
//! @param registry
//! @param npc_container_entity
//! @param reserved_sm If provided, un-reserves the container's position.
void destroy_npc_container( entt::registry &registry, entt::entity npc_container_entity,
                            const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr );

//! @brief Create a new NPC. Creates a new entity at `position_entity`. Returns the new entity.
//! @param registry
//! @param position_entity
//! @param npc_type
//! @param reserved_sm If provided, un-reserves the container's position when hatching from a container (nightwatchman/skeleton).
//! @return entt::entity
entt::entity create_npc( entt::registry &registry, entt::entity position_entity, const std::string &npc_type,
                         const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr );

//! @brief Remove an NPC's components (applying its DestroyAction stat modifiers to the player first).
//! @param registry
//! @param npc_entity
//! @return entt::null. Kept for symmetry with create_npc(); no loot entity is created here.
entt::entity destroy_npc( entt::registry &registry, entt::entity npc_entity );

//! @brief Create the death animation entity played when an NPC dies.
//! @param registry
//! @param npc_position_cmp
//! @param death_anim
//! @return The new death animation entity.
entt::entity create_npc_death_anim( entt::registry &registry, Cmp::Position npc_position_cmp, const Sprites::SpriteMetaType &death_anim );

//! @brief Remove the components added by create_npc_death_anim(), without destroying the entity.
//! @param registry
//! @param entity
void remove_npc_death_anim( entt::registry &registry, entt::entity entity );

//! @brief Emit a shockwave from an NPC (e.g. a priest) if its ShockwaveTimer has exceeded the configured frequency.
//! @param registry
//! @param npc_entt Entity carrying the Cmp::Npc::ShockwaveTimer and Cmp::Position.
//! @return true if a shockwave entity was created.
bool create_shockwave( entt::registry &registry, entt::entity npc_entt );

} // namespace Game::Factory::Npc

#endif // SRC_FACTORY_NPCFACTORY_HPP__
