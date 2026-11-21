#ifndef SRC_FACTORY_PLAYERFACTORY_HPP__
#define SRC_FACTORY_PLAYERFACTORY_HPP__

#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/fwd.hpp>

namespace Game::Factory::Player
{

//! @brief Create the player entity at its start position, with default stats.
//! @param registry
void create_player( entt::registry &registry );

//! @brief Reserve and decorate the player spawn area sprite, without blocking NPC pathfinding into it.
//! @param registry
//! @param entity
//! @param zorder
//! @return entt::entity
entt::entity add_spawn_area( entt::registry &registry, entt::entity entity, float zorder );

//! @brief Create the blood splat entity played when the player dies.
//! @param registry
//! @param player_pos_cmp
//! @param sprite
void create_player_death_anim( entt::registry &registry, Cmp::Position player_pos_cmp, const Sprites::SpriteSheet &sprite );

//! @brief Create an empty player inventory slot entity with the given index.
//! @param reg the ECS registry
//! @param idx the fixed slot index
//! @return entt::entity the new slot entity
entt::entity create_empty_inventory_slot( entt::registry &reg, uint8_t idx );

//! @brief Empty an inventory slot in place: reset its item and remove every per-item component. The slot entity and its
//! index are kept, so emptying a slot never shifts the other slots.
//! @note Read any item components you need (e.g. to copy them to a world item) before calling this.
//! @param reg the ECS registry
//! @param slot_entt the slot entity to empty
void clear_inventory_slot( entt::registry &reg, entt::entity slot_entt );

//! @brief Use up one item from an inventory slot: decrement its Cmp::Inventory::StackCount, or empty the slot (see
//! clear_inventory_slot()) when it holds its last item.
//! @param reg the ECS registry
//! @param slot_entt the slot entity to take from
void take_one_from_slot( entt::registry &reg, entt::entity slot_entt );

//! @brief Make sure there is exactly one inventory slot entity for each index `0..count-1`, creating missing slots empty
//! and removing slots beyond the current Cmp::Persist::InventorySlotCount.
//! @param reg the ECS registry
void ensure_inventory_slots( entt::registry &reg );

//! @brief Use up one item of the given type from a player inventory slot holding it, preferring the selected slot (see
//! take_one_from_slot(): a stack loses one item, a single item empties its slot).
//! See "item.xxxx" in res/json/items.json
//! @param reg the ECS registry
//! @param type the type to destroy
void destroy_inventory( entt::registry &reg, const Sys::ItemKey &type );

//! @brief Record the player's last graveyard position, so they can return to it later (e.g. after a crypt run).
//! @param reg
//! @param last_known_pos
//! @param offset Unused; kept for call-site symmetry.
//! @return `last_known_pos`, unchanged.
Cmp::Position add_player_last_graveyard_pos( entt::registry &reg, const Cmp::Position &last_known_pos,
                                             sf::Vector2f offset = { 0.f, Constants::kGridSizePxF.y } );

//! @brief Remove the player's recorded last graveyard position.
//! @param reg
void remove_player_last_graveyard_pos( entt::registry &reg );

//! @brief Consume the player's extra life: removes the ExtraLife component and its UI flash from the player, and
//! deactivates the scene's altars (see Factory::Altar::deactivate_altars()).
//! @param reg
void remove_player_extra_life( entt::registry &reg );

} // namespace Game::Factory::Player

#endif // SRC_FACTORY_PLAYERFACTORY_HPP__
