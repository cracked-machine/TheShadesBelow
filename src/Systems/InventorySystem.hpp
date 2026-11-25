#ifndef SRC_SYSTEMS_INVENTORYSYSTEM_HPP__
#define SRC_SYSTEMS_INVENTORYSYSTEM_HPP__

#include <Events/AddInventoryEvent.hpp>
#include <Events/DropInventoryEvent.hpp>
#include <Events/PickupWorldItemEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <SFML/System/Time.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/System/Clock.hpp>
#include <entt/entity/fwd.hpp>
#include <cstdint>
#include <optional>

// clang-format off
namespace Game::Events { struct PlayerActionEvent; struct SelectInventorySlotEvent; struct MoveInventorySlotEvent; }
// clang-format on

namespace Game::Sys
{

//! @brief Creates player inventory items, moves items between the player's inventory slots and the world, and changes
//! the selected slot. Event-driven, responding to Events::AddInventoryEvent, Events::DropInventoryEvent,
//! Events::PickupWorldItemEvent, Events::SelectInventorySlotEvent, Events::MoveInventorySlotEvent and the SWAP_INVENTORY/CONSUME actions of
//! Events::PlayerActionEvent. Slots always exist and never move; items fill and empty them in place.
class InventorySystem : public BaseSystem
{
public:
  //! @brief Construct a new Inventory System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  InventorySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointers for the pathfinding navmesh
  //! @param npc_navmesh
  //! @param player_navmesh
  //! @param reserved_sm
  //! @param item_sm Ground world items (see Scene::m_item_sm).
  void init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh, const PathFinding::SpatialHashGridSharedPtr &player_navmesh,
             const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &item_sm = nullptr )
  {
    m_npc_navmesh = npc_navmesh;
    m_player_navmesh = player_navmesh;
    m_reserved_sm = reserved_sm;
    m_item_sm = item_sm;
  }

  //! @brief Per-frame update function
  //! @param dt
  void update( sf::Time dt );

private:
  //! @brief Event handler for player actions; only responds to GameActions::DROP_INVENTORY
  //! @param event
  void on_player_action( const Events::PlayerActionEvent &event );

  //! @brief Event handler for Events::AddInventoryEvent. Calls add_inventory_item().
  //! @param ev
  void on_add_inventory_event( Events::AddInventoryEvent ev );

  //! @brief Single drop, no pickup
  //! @param ev
  void on_drop_inventory_event( Events::DropInventoryEvent ev );

  //! @brief Single pickup
  //! @param ev
  void on_pickup_world_item_event( Events::PickupWorldItemEvent ev );

  //! @brief Change the selected inventory slot. Ignored while eating or flying.
  //! @param ev
  void on_select_inventory_slot_event( const Events::SelectInventorySlotEvent &ev );

  //! @brief Move a slot's item to another slot by exchanging the two slot entities' indices, so every per-item
  //! component goes with its item: an empty destination takes the item, an occupied one swaps. The selection follows
  //! the moved item. Ignored while eating or flying.
  //! @param ev
  void on_move_inventory_slot_event( const Events::MoveInventorySlotEvent &ev );

  //! @brief Get the slot entity at `idx` (default: the selected slot), emptying it first if it unexpectedly holds an item.
  //! @param idx The slot index, or std::nullopt for the selected slot.
  //! @return entt::entity The empty slot entity, or entt::null if there is no slot with that index.
  entt::entity prepare_empty_slot( std::optional<uint8_t> idx );

  //! @brief handler for Events::UpdateDmgEvent. Calls update_dmg().
  //! @param ev
  void on_update_dmg_event( const Events::UpdateDmgEvent &ev );

  //! @brief Apply damage to item
  //! @param entt
  //! @param amount
  //! @param type
  void update_dmg( entt::entity entt, float amount, Events::UpdateDmgEvent::Type type );

  //! @brief Put `item` into an inventory slot, attaching any item-specific components
  //! (wear level if the item has one, UUID for torches).
  //! @param item Item identifier, e.g. "item.shovel". See res/json/items.json.
  //! @param idx The slot index, or std::nullopt for the selected slot.
  void add_inventory_item( const Sys::ItemKey &item, std::optional<uint8_t> idx = std::nullopt );

  //! @brief Standing on the held stack's item, pick it up into the stack as far as it fits (see merge_into_stack()).
  //! Otherwise drop one item from the selected slot's stack, or the whole stack (see drop_from_slot()), and once the
  //! slot is empty pick up the nearest world item (not one of the type just dropped) into it.
  //! @param whole_stack true to drop the whole stack (Shift+E), false to drop one item (E)
  void swap_inventory( bool whole_stack );

  //! @brief Drop count items from an inventory slot's stack at pos as a world item: drop_inventory_item() for the whole
  //! slot, split_from_slot() for part of its stack.
  //! @param pos the position to place a new world item
  //! @param inventory_slot_entt the player inventory slot entt
  //! @param count how many items to drop, capped at the slot's stack count
  void drop_from_slot( sf::Vector2f pos, entt::entity inventory_slot_entt, int count );

  //! @brief Split count items off an inventory slot's stack into a new world item at pos. The slot keeps the rest, and
  //! its UUID: the new item gets a UUID of its own, and a torch its own flame.
  //! @param pos the position to place the item
  //! @param inventory_slot_entt the player inventory slot entt
  //! @param count how many items to split off, less than the slot's stack count
  void split_from_slot( sf::Vector2f pos, entt::entity inventory_slot_entt, int count );

  //! @brief Create a world item at pos from an inventory slot's item, copying the components every dropped item keeps
  //! (anim, wear level, sting damage). The caller copies anything else and updates the slot.
  //! @param pos the position to place the item
  //! @param inventory_slot_entt the player inventory slot entt, which must hold an item
  //! @return entt::entity the new world item
  entt::entity create_world_item_from_slot( sf::Vector2f pos, entt::entity inventory_slot_entt );

  //! @brief Move an inventory slot's item into the world and empty the slot
  //! @param pos the postion to place the item
  //! @param inventory_slot_entt the player inventory slot entt
  void drop_inventory_item( sf::Vector2f pos, entt::entity inventory_slot_entt );

  //! @brief Fixed-step expiry tick for perishable inventory and world items. Wear is applied via
  //! update_dmg() (inventory) or Events::UpdateDmgEvent (world items); fully spoiled items are then replaced/removed.
  //! @param dt
  void update_item_expiry_damage( sf::Time dt );

  //! @brief Add the world item to an inventory slot and destroy the world entity.
  //! @param reg
  //! @param world_item_entt
  //! @param idx The slot index, or std::nullopt for the selected slot.
  void pickup_world_item( entt::registry &reg, entt::entity world_item_entt, std::optional<uint8_t> idx = std::nullopt );

  //! @brief Move as much of a world item's stack as fits into an inventory slot's stack (see Utils::Player::can_stack()).
  //! The world item keeps any leftover count, or is destroyed once it has all been taken.
  //! @param slot_entt The inventory slot whose stack it joins.
  //! @param world_item_entt The world item to take from.
  //! @return bool true if any of it was taken.
  bool merge_into_stack( entt::entity slot_entt, entt::entity world_item_entt );

  void try_consume_item();

  //! @brief Eat the selected inventory item if it is edible or drinkable.
  //! @param dt Update the EatingTimeAccumulator with the delta time.
  void consume_inventory( sf::Time dt );

  //! @brief Pause the graveyard music and loop the ElderFlute music in its place. No-op if already playing.
  void play_elder_flute_music();

  //! @brief Stop the ElderFlute music and resume the graveyard music. No-op if the ElderFlute music is not playing.
  void stop_elder_flute_music();

  //! @brief All grid positions that block NPC pathfinding
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief All grid positions that block player movement
  PathFinding::SpatialHashGridWeakPtr m_player_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;

  //! @brief Ground world items (see Scene::m_item_sm).
  PathFinding::SpatialHashGridWeakPtr m_item_sm;

  //! @brief Time since the last pickup/drop; swaps are ignored until Cmp::Persist::InventorySwapCooldown has passed.
  sf::Time m_swap_item_cooldown_timer;

  //! @brief fixed step for updating the item damage for expiry.
  sf::Time m_expiry_update_timer;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_INVENTORYSYSTEM_HPP__
