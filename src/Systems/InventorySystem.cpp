
#include <Audio/SoundBank.hpp>
#include <Components/AnimData.hpp>
#include <Components/Crypt/Chest.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Inventory/Explosive.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/ScryingBall.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LastDirection.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/EatingTimeAccumulator.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/DropInventoryEvent.hpp>
#include <Events/PickupWorldItemEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PlantFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SFML/Audio/SoundChannel.hpp>
#include <Systems/InventorySystem.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Utils/Cardinal.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/System/Time.hpp>
#include <spdlog/spdlog.h>

namespace Game::Sys
{

InventorySystem::InventorySystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory,
                                  Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank )
{
  // The entt::dispatcher is independent of the registry, so it is safe to bind event handlers in
  // the constructor
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&InventorySystem::on_player_action>( this );
  std::ignore = get_systems_event_queue().sink<Events::DropInventoryEvent>().connect<&InventorySystem::on_drop_inventory_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::PickupWorldItemEvent>().connect<&InventorySystem::on_pickup_world_item_event>( this );
  SPDLOG_DEBUG( "InventorySystem initialized" );
}

void InventorySystem::update( sf::Time dt )
{
  Factory::Particle::delete_expired_particle_sprites( reg(), "player.drop.particle.eating" );

  auto player_entt = Utils::Player::get_entity( reg() );
  if ( reg().any_of<Cmp::Player::EatingTimeAccumulator>( player_entt ) ) { consume_inventory( dt ); }
}

void InventorySystem::on_player_action( const Events::PlayerActionEvent &event )
{
  if ( event.action == Events::PlayerActionEvent::GameActions::SWAP_INVENTORY ) { swap_inventory(); }
}

void InventorySystem::on_drop_inventory_event( [[maybe_unused]] Events::DropInventoryEvent ev )
{
  auto [inventory_entt, _, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_entt == entt::null ) return;
  auto player_pos = Utils::Player::get_position( reg() ).position;
  drop_inventory_item( player_pos, inventory_entt );
}

void InventorySystem::on_pickup_world_item_event( Events::PickupWorldItemEvent ev ) { pickup_world_item( reg(), ev.world_item_entt ); }

void InventorySystem::swap_inventory()
{
  // if player is standing next to a Cmp::Crypt::Chest let them open it without dropping the inventory item
  for ( auto [chest_entt, chest_cmp, chest_pos_cmp] : reg().view<Cmp::Crypt::Chest, Cmp::Position>().each() )
  {
    if ( not Utils::Player::is_player_near( reg(), chest_pos_cmp ) ) continue;
    return;
  }

  if ( m_inventory_cooldown_timer.getElapsedTime() < sf::milliseconds( 750.f ) ) return;

  auto player_pos = Cmp::RectBounds::scaled( Utils::Player::get_position( reg() ), 0.5 );
  Sprites::SpriteMetaType existing_player_inventory_type;

  // drop inventory if we have one
  auto inventory_view = reg().view<Cmp::PlayerInventorySlot>();
  for ( auto [inventory_entt, inventory_cmp] : inventory_view.each() )
  {
    existing_player_inventory_type = inventory_cmp.m_item.sprite_type;
    drop_inventory_item( Utils::Player::get_position( reg() ).position, inventory_entt );
  }

  // pickup inventory if there is something at this position
  auto world_item_view = reg().view<Cmp::WorldItem, Cmp::Position>( entt::exclude<Cmp::PlantMultiBlock> );
  for ( auto [world_item_entt, world_item_cmp, world_item_pos_cmp] : world_item_view.each() )
  {
    if ( not player_pos.findIntersection( world_item_pos_cmp ) ) continue;        // is there something to pick up?
    if ( world_item_cmp.sprite_type == existing_player_inventory_type ) continue; // dont pick up the one we just dropped
    if ( inventory_view.size() > 0 ) { break; }                                   // don't pickup another if we already have one

    // ok pick it up
    SPDLOG_DEBUG( "GameActions::SWAP_INVENTORY calling 'pickup_world_item' with entt id {} ", static_cast<uint32_t>( carryitem_entt ) );
    Utils::Player::apply_action_from_world_item<Cmp::SpawnAction>( reg(), world_item_entt );
    pickup_world_item( reg(), world_item_entt );
  }
  m_inventory_cooldown_timer.restart();
  SPDLOG_DEBUG( "inventory_view: {} ", inventory_view.size() );
}

void InventorySystem::drop_inventory_item( sf::Vector2f pos, entt::entity inventory_slot_entt )
{
  auto *inventory_slot_cmp = reg().try_get<Cmp::PlayerInventorySlot>( inventory_slot_entt );

  if ( not inventory_slot_cmp )
  {
    SPDLOG_INFO( "Player has no inventory" );
    return;
  }

  // if player inventory is a plant item then replant it in the ground - snap to nearest grid to prevent collision issues
  if ( inventory_slot_cmp->m_item.sprite_type.contains( "plant" ) )
  {
    // multiblocks are top-left anchored, so offset the y-axis so that plant base is at players feet
    auto plant_pos = Utils::snap_to_grid( { pos.x, pos.y - Constants::kGridSizePxF.y } );

    // don't allow plants to be dropped at all in the player spawn area
    if ( Utils::Player::is_in_spawn( reg(), Cmp::Position{ plant_pos, Constants::kGridSizePxF } ) ) return;

    // for plant drops fall through to the rest of the function that creates normal world items
    if ( not inventory_slot_cmp->m_item.sprite_type.contains( "drop" ) )
    {
      auto [mb_entt, segment_entt_list] = Factory::Multiblock::add_multiblock_with_segments<Cmp::PlantMultiBlock, Cmp::PlantSegment>(
          reg(), plant_pos, m_sprite_factory.get_spritesheet_by_type( inventory_slot_cmp->m_item.sprite_type ), 0, 0, m_reserved_sm.lock().get() );
      // Preserve the item this plant was grown from, so digging it back up (see the DIG handler in
      // on_player_action_event) can hand it back via the normal pickup_world_item path instead of
      // having to re-derive an item id from the multiblock's sprite.
      reg().emplace_or_replace<Cmp::WorldItem>( mb_entt, inventory_slot_cmp->m_item );

      // rebuild the m_player_navmesh here
      if ( auto player_navmesh = m_player_navmesh.lock() )
      {
        player_navmesh->clear();
        for ( auto [entt, pos_cmp] : reg().view<Cmp::Player::NoPath, Cmp::Position>().each() )
        {
          player_navmesh->insert( entt, pos_cmp );
        }
      }

      // The NPC navmesh is built once at scene setup, so evict the tiles the new plant
      // now blocks. Keep the player entity - PlayerSystem::update re-inserts it every
      // frame and NPCs need it to path towards the player.
      if ( auto npc_navmesh = m_npc_navmesh.lock() )
      {
        for ( auto seg_entt : segment_entt_list )
        {
          if ( not reg().any_of<Cmp::Npc::NoPathFinding>( seg_entt ) ) continue;
          auto seg_pos_cmp = reg().get<Cmp::Position>( seg_entt );
          for ( auto blocked_entt : npc_navmesh->at( seg_pos_cmp ) )
          {
            if ( reg().any_of<Cmp::Player::Character>( blocked_entt ) ) continue;
            npc_navmesh->remove( blocked_entt, seg_pos_cmp );
          }
        }
      }

      // clear player inevntory
      reg().destroy( inventory_slot_entt );
      m_sound_bank.get_effect( "chopping_final" ).play();
      return;
    }
  }

  // otherwise just drop it as a world item
  auto world_item_entt = reg().create();
  reg().emplace_or_replace<Cmp::Position>( world_item_entt, pos, Constants::kGridSizePxF );
  // clang-format off
  reg().emplace_or_replace<Cmp::AnimData>( world_item_entt, Cmp::AnimData::Config{
        .sprite_type =  inventory_slot_cmp->m_item.sprite_type,
        .enabled = false
  });
  // clang-format on
  reg().emplace_or_replace<Cmp::ZOrderValue>( world_item_entt, pos.y - 1.f );
  reg().emplace_or_replace<Cmp::WorldItem>( world_item_entt, inventory_slot_cmp->m_item );
  reg().emplace_or_replace<Cmp::Npc::NoPathFinding>( world_item_entt );

  // try to copy any relevant components over to the new world carryitem entt
  auto *inventory_slot_level_cmp = reg().try_get<Cmp::Inventory::WearLevel>( inventory_slot_entt );
  if ( inventory_slot_level_cmp ) { reg().emplace_or_replace<Cmp::Inventory::WearLevel>( world_item_entt, inventory_slot_level_cmp->m_level ); }

  auto *inventory_scryingball_cmp = reg().try_get<Cmp::SeeingStone>( inventory_slot_entt );
  if ( inventory_scryingball_cmp ) { reg().emplace_or_replace<Cmp::SeeingStone>( world_item_entt, true, inventory_scryingball_cmp->target ); }

  auto *inventory_explosive_cmp = reg().try_get<Cmp::Explosive>( inventory_slot_entt );
  if ( inventory_explosive_cmp ) { reg().emplace_or_replace<Cmp::Explosive>( world_item_entt, false ); }

  auto *uuid_cmp = reg().try_get<Cmp::UUID>( inventory_slot_entt );
  if ( uuid_cmp )
  {

    for ( auto [ps_entt, ps_owner, ps_uuid_cmp] : reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
    {
      if ( ps_uuid_cmp == *uuid_cmp )
      {
        ps_owner.sprite->set_view_type( Cmp::Particle::ViewType::WORLD );
        ps_owner.sprite->set_scale( Cmp::Particle::kWorldScalePreset );
        reg().emplace_or_replace<Cmp::ZOrderValue>( ps_entt, Utils::Player::get_position( reg() ).y() - 1 );
      }
    }
    reg().emplace_or_replace<Cmp::UUID>( world_item_entt, uuid_cmp->data );
  }

  // now destroy the inventory slot
  reg().destroy( inventory_slot_entt );
  if ( world_item_entt != entt::null ) { m_sound_bank.get_effect( "drop_inventory" ).play(); }
}

void InventorySystem::pickup_world_item( entt::registry &reg, entt::entity world_item_entt )
{

  auto *anim_data_cmp = reg.try_get<Cmp::AnimData>( world_item_entt );
  if ( not anim_data_cmp ) return;

  auto *world_item_cmp = reg.try_get<Cmp::WorldItem>( world_item_entt );
  if ( not world_item_cmp )
    throw std::runtime_error( "InventorySystem::pickup_world_item - Unable to get world item component from " +
                              std::to_string( static_cast<uint32_t>( world_item_entt ) ) );

  // create the basic inventory slot entt
  auto inventory_entity = reg.create();
  reg.emplace_or_replace<Cmp::PlayerInventorySlot>( inventory_entity, *world_item_cmp );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( inventory_entity, Cmp::AnimData::Config{
        .sprite_type =  world_item_cmp->sprite_type,
        .enabled = false
  });
  // clang-format on

  // transfer any component properties from the world item that we want to retain before it is destroyed
  auto *uuid_cmp = reg.try_get<Cmp::UUID>( world_item_entt );
  if ( uuid_cmp )
  {
    for ( auto [ps_entt, ps_owner, ps_uuid_cmp] : reg.view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
    {
      if ( ps_uuid_cmp == *uuid_cmp )
      {
        // Move the ParticleSprite to the UI view. Reset the scale, zorder and view type.
        ps_owner.sprite->clear();
        ps_owner.sprite->set_scale( Cmp::Particle::kUiScalePreset );
        ps_owner.sprite->set_view_type( Cmp::Particle::ViewType::SCREEN );
        reg.emplace_or_replace<Cmp::ZOrderValue>( ps_entt, 50000 );
      }
    }
    reg.emplace_or_replace<Cmp::UUID>( inventory_entity, uuid_cmp->data );
  }

  auto *wear_level_cmp = reg.try_get<Cmp::Inventory::WearLevel>( world_item_entt );
  if ( wear_level_cmp ) { reg.emplace_or_replace<Cmp::Inventory::WearLevel>( inventory_entity, wear_level_cmp->m_level ); }

  auto *scryingball_cmp = reg.try_get<Cmp::SeeingStone>( world_item_entt );
  if ( scryingball_cmp ) { reg.emplace_or_replace<Cmp::SeeingStone>( inventory_entity, false, scryingball_cmp->target ); }

  auto *explosive_cmp = reg.try_get<Cmp::Explosive>( world_item_entt );
  if ( explosive_cmp ) { reg.emplace_or_replace<Cmp::Explosive>( inventory_entity, false ); }

  // now destroy the world item entt
  Factory::Plant::remove_plant_mb( reg, world_item_entt, m_npc_navmesh.lock(), m_player_navmesh.lock(), m_reserved_sm.lock() );
  if ( reg.valid( world_item_entt ) )
  {
    if ( auto reserved_sm = m_reserved_sm.lock(); reserved_sm )
    {
      if ( auto *pos_cmp = reg.try_get<Cmp::Position>( world_item_entt ) ) reserved_sm->remove( world_item_entt, *pos_cmp );
    }
    reg.destroy( world_item_entt );
  }

  if ( inventory_entity != entt::null ) { m_sound_bank.get_effect( "equip_inventory" ).play(); }
}

void InventorySystem::consume_inventory( sf::Time dt )
{
  auto player_entt = Utils::Player::get_entity( reg() );
  auto &eating_time = reg().get<Cmp::Player::EatingTimeAccumulator>( player_entt );
  static sf::Time eating_timeout = sf::milliseconds( 3000 );

  if ( eating_time < eating_timeout )
  {
    // stll eating
    if ( m_sound_bank.get_effect( "eating" ).getStatus() != sf::Sound::Status::Playing ) { m_sound_bank.get_effect( "eating" ).play(); }
    eating_time += dt;

    auto uuid = Cmp::UUID::generate();
    auto player_pos = Utils::Player::get_position( reg() ).getCenter();
    auto adj_player_pos = sf::Vector2f( player_pos.x, player_pos.y + 3 );
    auto player_zorder_pos = Utils::Player::get_position( reg() ).position.y;
    constexpr auto kParticleCount = 1;
    constexpr auto kLifetimeSeconds = 1.f;
    constexpr auto kSpeed = 25.f;
    constexpr auto kSize = 5.f;
    auto last_direction = Utils::Player::get_last_direction( reg() );
    if ( last_direction == Utils::Cardinal( Utils::Cardinal::North ).vector() )
    {
      Factory::Particle::add_eatingcrumbs_ps( reg(), "player.drop.particle.eating", kParticleCount, kLifetimeSeconds, kSpeed, kSize, uuid,
                                              adj_player_pos, last_direction, player_zorder_pos - 1.f );
    }
    else if ( last_direction == Utils::Cardinal( Utils::Cardinal::East ).vector() )
    {
      Factory::Particle::add_eatingcrumbs_ps( reg(), "player.drop.particle.eating", kParticleCount, kLifetimeSeconds, kSpeed, kSize, uuid,
                                              { adj_player_pos.x + 2, adj_player_pos.y }, last_direction, player_zorder_pos + 1.f );
    }
    else if ( last_direction == Utils::Cardinal( Utils::Cardinal::West ).vector() )
    {
      Factory::Particle::add_eatingcrumbs_ps( reg(), "player.drop.particle.eating", kParticleCount, kLifetimeSeconds, kSpeed, kSize, uuid,
                                              { adj_player_pos.x - 2, adj_player_pos.y }, last_direction, player_zorder_pos + 1.f );
    }
    else if ( last_direction == Utils::Cardinal( Utils::Cardinal::South ).vector() )
    {
      Factory::Particle::add_eatingcrumbs_ps( reg(), "player.drop.particle.eating", kParticleCount, kLifetimeSeconds, kSpeed, kSize, uuid,
                                              adj_player_pos, last_direction, player_zorder_pos + 1.f );
    }
  }
  else
  {
    // all done
    m_sound_bank.get_effect( "eating" ).stop();
    reg().remove<Cmp::Player::EatingTimeAccumulator>( player_entt );
    Utils::Player::apply_action_from_inventory_item<Cmp::ConsumeAction>( reg() );
    auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
    Factory::Player::destroy_inventory( reg(), inventory_type );
  }
}

} // namespace Game::Sys
