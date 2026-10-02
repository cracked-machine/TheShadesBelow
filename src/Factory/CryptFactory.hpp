#ifndef SRC_FACTORY_CRYPTFACTORY_HPP__
#define SRC_FACTORY_CRYPTFACTORY_HPP__

#include <Components/Crypt/RoomClosed.hpp>
#include <Components/Crypt/RoomEnd.hpp>
#include <Components/Crypt/RoomOpen.hpp>
#include <Components/Crypt/RoomStart.hpp>
#include <Components/Position.hpp>
#include <Components/Wall.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/Stores/StoreKey.hpp>

namespace Game::Sprites
{
class SpriteSheet;
} // namespace Game::Sprites

#include <entt/entity/fwd.hpp>

namespace Game::Factory::Crypt
{

//! @brief Create the crypt level exit tile. Only one exit may exist per crypt level.
//! @param reg
//! @param spawn_pos_px
//! @return The new exit entity.
//! @throws std::runtime_error if an exit tile already exists in the registry.
entt::entity create_crypt_exit( entt::registry &reg, sf::Vector2f spawn_pos_px );

//! @brief Create a crypt lever entity, used to open/close crypt rooms and passages.
//! @param reg
//! @param pos
//! @param sprite_type
//! @param sprite_idx
//! @param zorder
//! @return The new lever entity.
entt::entity create_crypt_lever( entt::registry &reg, sf::Vector2f pos, Sys::SpriteKey sprite_type, unsigned int sprite_idx, float zorder );

//! @brief Remove a crypt lever's components and destroy its entity.
//! @param reg
//! @param entt
void destroy_crypt_lever( entt::registry &reg, entt::entity entt );

//! @brief Create a lava pit hazard inside a crypt room, spawning one hazard cell per interior world tile.
//! @param reg
//! @param room Room bounds; the lava pit occupies the interior with a one-tile border left clear.
//! @param pathfinding_navmesh Optional navmesh to remove the lava cell positions from.
void create_crypt_lava_pit( entt::registry &reg, const Cmp::Crypt::RoomOpen &room,
                            std::shared_ptr<Game::PathFinding::SpatialHashGrid> pathfinding_navmesh );

//! @brief Destroy a lava pit and its hazard cells, restoring their positions in `pathfinding_navmesh`.
//! @param reg
//! @param entt The RoomLavaPit entity.
//! @param pathfinding_navmesh Optional navmesh to remove the destroyed cell entities from.
void destroy_crypt_lava_pit( entt::registry &reg, entt::entity entt, std::shared_ptr<Game::PathFinding::SpatialHashGrid> pathfinding_navmesh );

//! @brief Add a spike trap entity at the position of a passage block belonging to `passage_id`.
//! @param reg
//! @param entt Entity carrying the Cmp::Crypt::PassageBlock to place the trap at.
//! @param passage_id Passage this spike trap belongs to.
void add_spike_trap( entt::registry &reg, const entt::entity entt, const int passage_id );

//! @brief Create a crypt loot chest entity.
//! @param reg
//! @param pos
//! @param sprite_type
//! @param sprite_idx
//! @param zorder
//! @return The new chest entity.
entt::entity create_crypt_chest( entt::registry &reg, sf::Vector2f pos, Sys::SpriteKey sprite_type, unsigned int sprite_idx, float zorder );

//! @brief Remove a crypt chest's components and destroy its entity.
//! @param reg
//! @param entt
void destroy_crypt_chest( entt::registry &reg, entt::entity entt );

//! @brief Create the singleton timer entity that periodically reshuffles the crypt layout.
//! @param reg
//! @param threshold Timer duration, in seconds.
void create_crypt_shuffle_timer( entt::registry &reg, float threshold );

//! @brief Destroy the crypt shuffle timer entity, if one exists.
//! @param reg
void destroy_crypt_shuffle_timer( entt::registry &reg );

} // namespace Game::Factory::Crypt

#endif // SRC_FACTORY_CRYPTFACTORY_HPP__
