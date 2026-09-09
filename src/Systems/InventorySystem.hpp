#ifndef SRC_SYSTEMS_INVENTORYSYSTEM_HPP__
#define SRC_SYSTEMS_INVENTORYSYSTEM_HPP__

#include <Events/DropInventoryEvent.hpp>
#include <Events/PickupWorldItemEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/System/Clock.hpp>

namespace Game::Sys
{

//! @brief Moves items between the player's inventory slot and the world. Event-driven, responding to
//! Events::DropInventoryEvent, Events::PickupWorldItemEvent, and the DROP_INVENTORY action of
//! Events::PlayerActionEvent.
class InventorySystem : public BaseSystem
{
public:
  //! @brief Construct a new Inventory System object
  //! @param reg
  //! @param window
  //! @param sprite_factory
  //! @param sound_bank
  InventorySystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

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

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Event handler for player actions; only responds to GameActions::DROP_INVENTORY
  //! @param event
  void on_player_action( const Events::PlayerActionEvent &event );

  //! @brief Single drop, no pickup
  //! @param ev
  void on_drop_inventory_event( Events::DropInventoryEvent ev );

  //! @brief Single pickup
  //! @param ev
  void on_pickup_world_item_event( Events::PickupWorldItemEvent ev );

  //! @brief Drop the inventory (if player has one) and pickup the nearest workd item into inventory
  void swap_inventory();

  //! @brief Remove the CarryItem from player inventory and place it into the world
  //! @param pos the postion to place the item
  //! @param inventory_slot_entt the player inventory slot entt
  void drop_inventory_item( sf::Vector2f pos, entt::entity inventory_slot_entt );

  //! @brief Add the world item to the player's inventory and destroy the world entity.
  //! @param reg
  //! @param world_item_entt
  void pickup_world_item( entt::registry &reg, entt::entity world_item_entt );

  //! @brief Eat the inventory item if its a ".drop" item.
  //! @param dt Update the EatingTimeAccumulator with the delta time.
  void consume_inventory( sf::Time dt );

  //! @brief All grid positions that block NPC pathfinding
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief All grid positions that block player movement
  PathFinding::SpatialHashGridWeakPtr m_player_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;

  //! @brief Prevent player from spamming the drop inventory action.
  sf::Clock m_inventory_cooldown_timer;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_INVENTORYSYSTEM_HPP__
