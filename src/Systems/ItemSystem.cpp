
#include <Audio/SoundBank.hpp>
#include <Components/AnimData.hpp>
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
#include <Utils/Player.hpp>

#include <algorithm>

namespace Game::Sys
{

ItemSystem::ItemSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  SPDLOG_DEBUG( "ItemSystem initialized" );
  std::ignore = get_systems_event_queue().sink<Events::CreateItemEvent>().connect<&ItemSystem::on_create_item_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::UpdateDmgEvent>().connect<&ItemSystem::on_update_dmg_event>( this );
}

void ItemSystem::on_create_item_event( Game::Events::CreateItemEvent ev ) { create_world_item( ev.m_pos, ev.m_item, ev.m_sfx, ev.m_zorder ); }
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

    if ( wearlevel_cmp.m_level <= 0 )
    {
      const auto &rotten_item = Sys::ItemStore::instance().get( "item.rottenfood" );
      reg().emplace_or_replace<Cmp::WorldItem>( ev.m_item_entt, rotten_item );
      reg().emplace_or_replace<Cmp::AnimData>( ev.m_item_entt, Cmp::AnimData::Config{ .sprite_type = rotten_item.sprite_type, .enabled = false } );
      reg().remove<Cmp::Inventory::WearLevel>( ev.m_item_entt );
    }
  }
}

void ItemSystem::create_world_item( Cmp::Position pos, const Sys::ItemKey &item, std::string sfx, float zorder )
{

  if ( item == "item.bomb" )
  {
    create_explosive( pos, item, zorder );
    return;
  }

  auto world_item_entt = reg().create();
  Cmp::Position world_item_pos( pos.position, pos.size );
  reg().emplace_or_replace<Cmp::Position>( world_item_entt, world_item_pos );
  if ( auto reserved_sm = m_reserved_sm.lock() ) reserved_sm->insert( world_item_entt, world_item_pos );
  // clang-format off
  reg().emplace_or_replace<Cmp::AnimData>( world_item_entt, Cmp::AnimData::Config{ 
        .sprite_type =  Sys::ItemStore::instance().get( item ).sprite_type, 
        .enabled = true
  });
  // clang-format on
  reg().emplace_or_replace<Cmp::ZOrderValue>( world_item_entt, pos.position.y - 1.f + zorder );
  reg().emplace_or_replace<Cmp::Npc::NoPathFinding>( world_item_entt );
  // Use a UUID to identify the InventoryItem/PlayerInventorySlot when the entity is destroyed.
  reg().emplace_or_replace<Cmp::UUID>( world_item_entt, Cmp::UUID::generate() );
  if ( item == "item.axe" || item == "item.pickaxe" || item == "item.shovel" || item == "item.dowsingrod" )
  {
    reg().emplace_or_replace<Cmp::Inventory::WearLevel>( world_item_entt, 100.f );
  }
  // the target is picked once here, then carried with the item between the world and the player inventory
  if ( item == "item.dowsingrod" )
  {
    reg().emplace_or_replace<Cmp::Inventory::DowsingTarget>( world_item_entt, Cmp::Inventory::DowsingTarget::random_pick( {} ) );
  }

  reg().emplace_or_replace<Cmp::WorldItem>( world_item_entt, Sys::ItemStore::instance().get( item ) );

  SPDLOG_INFO( "Placed {} at {},{}", item, pos.position.x, pos.position.y );
  if ( world_item_entt != entt::null and not sfx.empty() ) { m_sound_bank.get_effect( sfx ).play(); }
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