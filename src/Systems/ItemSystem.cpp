
#include <Audio/SoundBank.hpp>
#include <Components/AnimData.hpp>
#include <Components/DropCount.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Inventory/DowsingTarget.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Position.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/ItemSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <algorithm>

namespace Game::Sys
{

ItemSystem::ItemSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  SPDLOG_DEBUG( "ItemSystem initialized" );
  std::ignore = get_systems_event_queue().sink<Events::CreateItemEvent>().connect<&ItemSystem::on_create_item_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::SwapWorldItemEvent>().connect<&ItemSystem::on_swap_world_item_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::UpdateDmgEvent>().connect<&ItemSystem::on_update_dmg_event>( this );
}

void ItemSystem::on_create_item_event( Game::Events::CreateItemEvent ev )
{
  if ( not ev.m_spawn_nearby )
  {
    auto item_entt = create_world_item( ev.m_pos, ev.m_item, ev.m_sfx, ev.m_zorder );
    if ( item_entt != entt::null and ev.m_on_created ) { ev.m_on_created( item_entt ); }
    return;
  }

  const bool has_source = reg().valid( ev.m_source_entt );
  if ( has_source and ev.m_max_instances > 0 )
  {
    const auto *drop_count_cmp = reg().try_get<Cmp::DropCount>( ev.m_source_entt );
    if ( drop_count_cmp and drop_count_cmp->m_count >= ev.m_max_instances ) return;
  }

  auto npc_navmesh = m_npc_navmesh.lock();
  auto ghost_navmesh = m_ghost_navmesh.lock();
  auto loot_sm = m_loot_sm.lock();
  auto reserved_sm = m_reserved_sm.lock();
  const Utils::SpawnGrids grids{ npc_navmesh.get(), ghost_navmesh.get(), loot_sm.get(), reserved_sm.get() };
  auto cell = Utils::find_random_cell_near( ev.m_pos, kItemDropSearchRadius,
                                            [&]( const sf::Vector2f &cell_pos )
                                            { return Utils::is_free_spawn_area( reg(), sf::FloatRect( cell_pos, Constants::kGridSizePxF ), grids ); } );
  if ( not cell )
  {
    SPDLOG_WARN( "No free cell to drop {} near [{},{}].", ev.m_item.str(), ev.m_pos.position.x, ev.m_pos.position.y );
    return;
  }

  auto item_entt = create_world_item( Cmp::Position( *cell, Constants::kGridSizePxF ), ev.m_item, ev.m_sfx, ev.m_zorder );
  // counted over the source's lifetime, so picking the item up doesn't free a slot
  if ( item_entt != entt::null and has_source ) { reg().get_or_emplace<Cmp::DropCount>( ev.m_source_entt ).m_count++; }
  if ( item_entt != entt::null and ev.m_on_created ) { ev.m_on_created( item_entt ); }
}

void ItemSystem::on_swap_world_item_event( const Events::SwapWorldItemEvent &ev ) { swap_world_item( ev.m_entt, ev.m_new_item, ev.m_sfx ); }
void ItemSystem::on_update_dmg_event( const Events::UpdateDmgEvent &ev )
{
  // don't assume the item has a wear level
  if ( reg().all_of<Cmp::WorldItem, Cmp::Inventory::WearLevel>( ev.m_item_entt ) )
  {
    auto &wearlevel_cmp = reg().get<Cmp::Inventory::WearLevel>( ev.m_item_entt );
    switch ( ev.m_type )
    {
      case Events::UpdateDmgEvent::ADD:
        wearlevel_cmp.m_level = std::clamp( wearlevel_cmp.m_level + ev.m_amount, 0.f, 100.f );
        break;
      case Events::UpdateDmgEvent::SUBTRACT:
        wearlevel_cmp.m_level = std::clamp( wearlevel_cmp.m_level - ev.m_amount, 0.f, 100.f );
        break;
    }

    // depleted plants stay in the world, they just can't be foraged any more
    if ( wearlevel_cmp.m_level <= 0 and not reg().all_of<Cmp::PlantMultiBlock>( ev.m_item_entt ) )
    {
      swap_world_item( ev.m_item_entt, "item.rottenfood" );
    }
  }
}

entt::entity ItemSystem::create_world_item( Cmp::Position pos, const Sys::ItemKey &item, std::string sfx, float zorder )
{

  if ( item == "item.bomb" )
  {
    create_explosive( pos, item, zorder );
    return entt::null;
  }

  const auto &world_item = Sys::ItemStore::instance().get( item );

  auto world_item_entt = reg().create();
  Cmp::Position world_item_pos( pos.position, pos.size );
  reg().emplace_or_replace<Cmp::Position>( world_item_entt, world_item_pos );
  if ( auto reserved_sm = m_reserved_sm.lock() ) reserved_sm->insert( world_item_entt, world_item_pos );
  // clang-format off
  reg().emplace_or_replace<Cmp::AnimData>( world_item_entt, Cmp::AnimData::Config{ 
        .sprite_type =  world_item.sprite_type, 
        .enabled = true
  });
  // clang-format on
  reg().emplace_or_replace<Cmp::ZOrderValue>( world_item_entt, pos.position.y - 1.f + zorder );
  reg().emplace_or_replace<Cmp::Npc::NoPathFinding>( world_item_entt );
  // Use a UUID to identify the InventoryItem/PlayerInventorySlot when the entity is destroyed.
  reg().emplace_or_replace<Cmp::UUID>( world_item_entt, Cmp::UUID::generate() );
  if ( world_item.has_wear() ) { reg().emplace_or_replace<Cmp::Inventory::WearLevel>( world_item_entt, 100.f ); }
  // the target is picked once here, then carried with the item between the world and the player inventory
  if ( item == "item.dowsingrod" )
  {
    reg().emplace_or_replace<Cmp::Inventory::DowsingTarget>( world_item_entt, Cmp::Inventory::DowsingTarget::random_pick( {} ) );
  }

  reg().emplace_or_replace<Cmp::WorldItem>( world_item_entt, world_item );

  SPDLOG_INFO( "Placed {} at {},{}", item, pos.position.x, pos.position.y );
  if ( world_item_entt != entt::null and not sfx.empty() ) { m_sound_bank.get_effect( sfx ).play(); }
  return world_item_entt;
}

void ItemSystem::swap_world_item( entt::entity entt, const Sys::ItemKey &item, const std::string &sfx )
{
  if ( not reg().valid( entt ) or not reg().any_of<Cmp::WorldItem>( entt ) )
  {
    SPDLOG_WARN( "Cannot swap to {}: entity {} is not a world item", item, static_cast<uint32_t>( entt ) );
    return;
  }

  const auto &new_item = Sys::ItemStore::instance().get( item );
  SPDLOG_INFO( "Swapped {} for {}", reg().get<Cmp::WorldItem>( entt ).item_type, item );
  reg().emplace_or_replace<Cmp::WorldItem>( entt, new_item );
  reg().emplace_or_replace<Cmp::AnimData>( entt, Cmp::AnimData::Config{ .sprite_type = new_item.sprite_type, .enabled = false } );
  if ( new_item.has_wear() ) { reg().emplace_or_replace<Cmp::Inventory::WearLevel>( entt, 100.f ); }
  else { reg().remove<Cmp::Inventory::WearLevel>( entt ); }

  if ( not sfx.empty() ) { m_sound_bank.get_effect( sfx ).play(); }
}

void ItemSystem::create_explosive( Cmp::Position pos, const Sys::ItemKey &item, float zorder )
{
  // Now create the entity with the valid target
  auto world_carry_item_entt = reg().create();
  reg().emplace_or_replace<Cmp::Position>( world_carry_item_entt, pos.position, pos.size );
  // clang-format off
  reg().emplace_or_replace<Cmp::AnimData>( world_carry_item_entt, Cmp::AnimData::Config{  
        .sprite_type =  Sys::ItemStore::instance().get( item ).sprite_type
  });
  // clang-format on
  reg().emplace_or_replace<Cmp::ZOrderValue>( world_carry_item_entt, pos.position.y - 1.f + zorder );
  reg().emplace_or_replace<Cmp::WorldItem>( world_carry_item_entt, Sys::ItemStore::instance().get( item ) );
  reg().emplace_or_replace<Cmp::Npc::NoPathFinding>( world_carry_item_entt );

  SPDLOG_INFO( "Placed {} at {},{}", item, pos.position.x, pos.position.y );
}

} // namespace Game::Sys