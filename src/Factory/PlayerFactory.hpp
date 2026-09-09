#ifndef SRC_FACTORY_PLAYERFACTORY_HPP__
#define SRC_FACTORY_PLAYERFACTORY_HPP__

#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <Factory/SpriteFactory.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/fwd.hpp>

namespace Game::Factory::Player
{

//! @brief Create the player entity at its start position, with default stats and a starting pickaxe.
//! @param registry
//! @param reserved_sm If provided, reserves the player's position.
void create_player( entt::registry &registry, const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr );

//! @brief Reserve and decorate the player spawn area sprite, without blocking NPC pathfinding into it.
//! @param registry
//! @param entity
//! @param sfactory Used to pick a random spawn-area sprite index.
//! @param zorder
//! @return entt::entity
entt::entity add_spawn_area( entt::registry &registry, entt::entity entity, Sprites::SpriteFactory &sfactory, float zorder );

//! @brief Create the blood splat entity played when the player dies.
//! @param registry
//! @param player_pos_cmp
//! @param sprite
void create_player_death_anim( entt::registry &registry, Cmp::Position player_pos_cmp, const Sprites::SpriteSheet &sprite );

//! @brief Destroy all player inventory slots matching a type. See "item.xxxx" in res/json/items.json
//! @param reg the ECS registry
//! @param type the type to destroy
void destroy_inventory( entt::registry &reg, const std::string &type );

//! @brief Create a new player inventory slot entity for `item`, attaching any item-specific components
//! (wear level for axe/shovel, seeing stone target for scrying balls, etc).
//! @param reg
//! @param item Item identifier, e.g. "item.pickaxe". See res/json/sprite_metadata.json.
void add_inventory( entt::registry &reg, const std::string &item );

//! @brief Record the player's last graveyard position, so they can return to it later (e.g. after a crypt run).
//! @param reg
//! @param last_known_pos
//! @param offset Unused; kept for call-site symmetry.
//! @return `last_known_pos`, unchanged.
Cmp::Position add_player_last_graveyard_pos( entt::registry &reg, Cmp::Position &last_known_pos,
                                             sf::Vector2f offset = { 0.f, Constants::kGridSizePxF.y } );

//! @brief Remove the player's recorded last graveyard position.
//! @param reg
void remove_player_last_graveyard_pos( entt::registry &reg );

//! @brief Consume the player's extra life: removes the ExtraLife component, its UI flash, and the crypt
//! altar particle effect, and deactivates the crypt altar sprite.
//! @param reg
void remove_player_extra_life( entt::registry &reg );

} // namespace Game::Factory::Player

#endif // SRC_FACTORY_PLAYERFACTORY_HPP__
