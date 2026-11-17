#ifndef SRC_SYSTEMS_INVENTORYSYSTEM_HPP__
#define SRC_SYSTEMS_INVENTORYSYSTEM_HPP__

#include <Events/AddInventoryEvent.hpp>
#include <Events/DropInventoryEvent.hpp>
#include <Events/PickupWorldItemEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Events/SelectInventorySlotEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <SFML/System/Time.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/System/Clock.hpp>
#include <entt/entity/fwd.hpp>
#include <cstdint>
#include <optional>

namespace Game::Sys
{

//! @brief Creates player inventory items, moves items between the player's inventory slots and the world, and changes
//! the selected slot. Event-driven, responding to Events::AddInventoryEvent, Events::DropInventoryEvent,
//! Events::PickupWorldItemEvent, Events::SelectInventorySlotEvent and the SWAP_INVENTORY/CONSUME actions of
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
  void init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh, const PathFinding::SpatialHashGridSharedPtr &player_navmesh,
             const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr )
  {
    m_npc_navmesh = npc_navmesh;
    m_player_navmesh = player_navmesh;
    m_reserved_sm = reserved_sm;
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

  //! @brief Drop the selected slot's item (if any) and pickup the nearest world item into the selected slot
  void swap_inventory();

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

  //! @brief Time since the last pickup/drop; swaps are ignored until Cmp::Persist::InventorySwapCooldown has passed.
  sf::Time m_swap_item_cooldown_timer;

  //! @brief fixed step for updating the item damage for expiry.
  sf::Time m_expiry_update_timer;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_INVENTORYSYSTEM_HPP__
