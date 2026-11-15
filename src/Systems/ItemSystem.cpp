
#include <Audio/SoundBank.hpp>
#include <Components/AnimData.hpp>
#include <Components/DropCount.hpp>
#include <Components/Grave/CurseTablet.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Inventory/DowsingTarget.hpp>
#include <Components/Inventory/StingDamage.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Persistent/AutoPickupDrops.hpp>
#include <Components/Position.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/DestroyitemEvent.hpp>
#include <Events/PickupWorldItemEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <Factory/ItemFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SFML/Audio/Sound.hpp>
#include <Systems/ItemSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <algorithm>
#include <entt/entity/entity.hpp>

namespace Game::Sys
{

ItemSystem::ItemSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  SPDLOG_DEBUG( "ItemSystem initialized" );
  std::ignore = get_systems_event_queue().sink<Events::CreateItemEvent>().connect<&ItemSystem::on_create_item_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::DestroyItemEvent>().connect<&ItemSystem::on_destroy_item_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::SwapWorldItemEvent>().connect<&ItemSystem::on_swap_world_item_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::UpdateDmgEvent>().connect<&ItemSystem::on_update_dmg_event>( this );
}

void ItemSystem::on_create_item_event( Game::Events::CreateItemEvent ev )
{
  // 1. where to spawn: the exact position, or a random free cell beside the source
  Cmp::Position spawn_pos = ev.m_pos;
  const bool has_source = reg().valid( ev.m_source_entt );
  if ( ev.m_spawn_nearby )
  {
    if ( has_source and ev.m_max_instances > 0 )
    {
      const auto *drop_count_cmp = reg().try_get<Cmp::DropCount>( ev.m_source_entt );
      if ( drop_count_cmp and drop_count_cmp->m_count >= ev.m_max_instances ) return;
    }

    auto npc_navmesh = m_npc_navmesh.lock();
    auto ghost_navmesh = m_ghost_navmesh.lock();
    auto loot_sm = m_loot_sm.lock();
    auto reserved_sm = m_reserved_sm.lock();
    const Utils::SpawnGrids grids{
        .navmesh = npc_navmesh.get(), .ghost_navmesh = ghost_navmesh.get(), .loot_sm = loot_sm.get(), .reserved_sm = reserved_sm.get() };
    auto cell = Utils::find_random_cell_near( ev.m_pos, kItemDropSearchRadius, [&]( const sf::Vector2f &cell_pos )
    { return Utils::is_free_spawn_area( reg(), sf::FloatRect( cell_pos, Constants::kGridSizePxF ), grids ); } );
    if ( not cell )
    {
      SPDLOG_WARN( "No free cell to drop {} near [{},{}].", ev.m_item.str(), ev.m_pos.position.x, ev.m_pos.position.y );
      return;
    }
    spawn_pos = Cmp::Position( *cell, Constants::kGridSizePxF );
  }

  // 2. which sprite: a random frame if the item's sprite.random is set, else its sprite.index
  const bool is_curse_tablet = ev.m_item.contains( "cursetablet" );
  const size_t sprite_idx = Sys::ItemStore::instance().get( ev.m_item ).sprite_index();

  // 3. create it
  const entt::entity item_entt = create_world_item( spawn_pos, ev.m_item, ev.m_sfx, sprite_idx, ev.m_zorder, ev.m_sting );
  if ( item_entt == entt::null ) return;

  // counted over the source's lifetime, so picking the item up doesn't free a slot
  if ( ev.m_spawn_nearby and has_source ) { reg().get_or_emplace<Cmp::DropCount>( ev.m_source_entt ).m_count++; }

  // 4. post-creation tagging
  if ( is_curse_tablet ) { Factory::Item::add_curse_tablet( reg(), item_entt, ev.m_item, sprite_idx ); }
  if ( ev.m_on_created ) { ev.m_on_created( item_entt ); }

  // 5. a reward for the player goes straight into an empty slot, if the setting allows. Done last so on_created's
  // bindings (e.g. a torch's flame, a summoning offering) are on the item to be carried over. Enqueued, as the caller
  // may still be iterating registry views; InventorySystem picks the slot then, or leaves the item on the ground
  if ( ev.m_auto_pickup and Sys::PersistSystem::get<Cmp::Persist::AutoPickupDrops>( reg() ).get_value() and
       not reg().all_of<Cmp::PlantMultiBlock>( item_entt ) )
  {
    get_systems_event_queue().enqueue( Events::PickupWorldItemEvent{ item_entt, /*to_empty_slot=*/true } );
  }
}

void ItemSystem::on_destroy_item_event( Game::Events::DestroyItemEvent ev )
{
  if ( not reg().valid( ev.m_item_entity ) ) return;
  SPDLOG_DEBUG( "Destroying world item {}", static_cast<uint32_t>( ev.m_item_entity ) );
  reg().destroy( ev.m_item_entity );
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

    // worn out: expired curse tablets vanish silently (the smash that breaks one is handled by
    // ActionSystem::check_player_smash_tablet, which never lets wear reach 0), plants stay depleted, everything else rots
    if ( wearlevel_cmp.m_level <= 0 )
    {
      if ( reg().any_of<Cmp::Grave::CurseTablet>( ev.m_item_entt ) )
      {
        get_systems_event_queue().trigger( Events::DestroyItemEvent( ev.m_item_entt ) );
      }
      else if ( not reg().any_of<Cmp::PlantMultiBlock>( ev.m_item_entt ) ) { swap_world_item( ev.m_item_entt, "item.rottenfood" ); }
    }

    return;
  }
}

entt::entity ItemSystem::create_world_item( Cmp::Position pos, const Sys::ItemKey &item, const std::string &sfx, size_t sprite_idx, float zorder,
                                            bool sting )
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
        .frame_index_offset = sprite_idx,
        .enabled = true
  });
  // clang-format on

  reg().emplace_or_replace<Cmp::ZOrderValue>( world_item_entt, pos.position.y - 1.f + zorder );
  reg().emplace_or_replace<Cmp::Npc::NoPathFinding>( world_item_entt );

  // Use a UUID to identify the InventoryItem/PlayerInventorySlot when the entity is destroyed.
  reg().emplace_or_replace<Cmp::UUID>( world_item_entt, Cmp::UUID::generate() );
  if ( world_item.has_wear() ) { reg().emplace_or_replace<Cmp::Inventory::WearLevel>( world_item_entt, 100.f ); }
  if ( sting and world_item.stings() ) { reg().emplace_or_replace<Cmp::Inventory::StingDamage>( world_item_entt ); }
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
  reg().emplace_or_replace<Cmp::AnimData>( entt, Cmp::AnimData::Config{ .sprite_type = new_item.sprite_type, .frame_index_offset = new_item.sprite_index(), .enabled = false } );
  if ( new_item.has_wear() ) { reg().emplace_or_replace<Cmp::Inventory::WearLevel>( entt, 100.f ); }
  else { reg().remove<Cmp::Inventory::WearLevel>( entt ); }
  if ( new_item.stings() ) { reg().emplace_or_replace<Cmp::Inventory::StingDamage>( entt ); }
  else { reg().remove<Cmp::Inventory::StingDamage>( entt ); }

  if ( not sfx.empty() ) { m_sound_bank.get_effect( sfx ).play(); }
}

void ItemSystem::create_explosive( Cmp::Position pos, const Sys::ItemKey &item, float zorder )
{
  // Now create the entity with the valid target
  auto world_carry_item_entt = reg().create();
  reg().emplace_or_replace<Cmp::Position>( world_carry_item_entt, pos.position, pos.size );
  const auto &world_item = Sys::ItemStore::instance().get( item );
  // clang-format off
  reg().emplace_or_replace<Cmp::AnimData>( world_carry_item_entt, Cmp::AnimData::Config{  
        .sprite_type = world_item.sprite_type,
        .frame_index_offset = world_item.sprite_index()
  });
  // clang-format on
  reg().emplace_or_replace<Cmp::ZOrderValue>( world_carry_item_entt, pos.position.y - 1.f + zorder );
  reg().emplace_or_replace<Cmp::WorldItem>( world_carry_item_entt, world_item );
  reg().emplace_or_replace<Cmp::Npc::NoPathFinding>( world_carry_item_entt );

  SPDLOG_INFO( "Placed {} at {},{}", item, pos.position.x, pos.position.y );
}

} // namespace Game::Sys