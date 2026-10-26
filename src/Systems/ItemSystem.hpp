#ifndef SRC_SYSTEMS_ITEMSYSTEM_HPP__
#define SRC_SYSTEMS_ITEMSYSTEM_HPP__

#include <Events/CreateItemEvent.hpp>
#include <Events/SwapWorldItemEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/StoreKey.hpp>

namespace Game::Cmp
{
class Position;
} // namespace Game::Cmp

namespace Game::Sys
{

//! @brief Spawns world item entities (dropped/placed items, seeing stones, explosives) in response to
//! Events::CreateItemEvent, and swaps existing world items in response to Events::SwapWorldItemEvent.
class ItemSystem : public BaseSystem
{
public:
  //! @brief Construct a new Item System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  ItemSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Destroy the Item System object
  ~ItemSystem() {}

  //! @brief init the weak pointer for the reserved-positions grid.
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &reserved_sm ) { m_reserved_sm = reserved_sm; }

  //! @brief Event handler that forwards to create_world_item().
  //! @param ev
  void on_create_item_event( Game::Events::CreateItemEvent ev );

  //! @brief Event handler that forwards to swap_world_item().
  //! @param ev
  void on_swap_world_item_event( const Events::SwapWorldItemEvent &ev );

  //! @brief Apply wear to the world item
  //! @param ev
  void on_update_dmg_event( const Events::UpdateDmgEvent &ev );

  //! @brief Create a generic world item entity, dispatching to create_explosive() for bombs.
  //! @param pos
  //! @param item Item identifier, e.g. "item.pickaxe". See res/json/sprite_metadata.json.
  //! @param sfx Sound effect to play on placement, if not empty.
  //! @param zorder
  void create_world_item( Cmp::Position pos, const Sys::ItemKey &item, std::string sfx, float zorder = 0.f );

  //! @brief Replace an existing world item in place with another item type, keeping its position, UUID, etc.
  //! @note The Cmp::Inventory::WearLevel is reset if the new item wears, otherwise it is removed.
  //! @param entt The world item entity to swap. No op if it is not a valid world item.
  //! @param item Item identifier of the replacement item, e.g. "item.lightningrod.broken".
  //! @param sfx Sound effect to play on swap, if not empty.
  void swap_world_item( entt::entity entt, const Sys::ItemKey &item, const std::string &sfx = "" );

  //! @brief Create an explosive (bomb) world item.
  //! @param pos
  //! @param item
  //! @param zorder
  void create_explosive( Cmp::Position pos, const Sys::ItemKey &item, float zorder );

private:
  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_ITEMSYSTEM_HPP__
