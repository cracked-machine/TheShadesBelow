
#include <Audio/SoundBank.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/AnimData.hpp>
#include <Components/Cart/MultiBlock.hpp>
#include <Components/Crypt/Chest.hpp>
#include <Components/Crypt/ObjectiveMultiBlock.hpp>
#include <Components/Grave/CurseTablet.hpp>
#include <Components/Health.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Horse/MultiBlock.hpp>
#include <Components/Inventory/DowsingTarget.hpp>
#include <Components/Inventory/FlashUIHealth.hpp>
#include <Components/Inventory/InventoryDrag.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/StackCount.hpp>
#include <Components/Inventory/StingDamage.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LastDirection.hpp>
#include <Components/Particle/SpriteBase.hpp>
#include <Components/Particle/SpriteOwner.hpp>
#include <Components/Persistent/InventorySlotRollover.hpp>
#include <Components/Persistent/InventoryStackSize.hpp>
#include <Components/Persistent/InventorySwapCooldown.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/SummonOffering.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/ConsumeTimer.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Player/SelectedInventorySlot.hpp>
#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/PlayerStats.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/AddInventoryEvent.hpp>
#include <Events/ConsumeRefusedEvent.hpp>
#include <Events/DestroyPlantEvent.hpp>
#include <Events/DropInventoryEvent.hpp>
#include <Events/ItemPickedUpEvent.hpp>
#include <Events/PickupWorldItemEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Events/DropInventorySlotEvent.hpp>
#include <Events/MoveInventorySlotEvent.hpp>
#include <Events/SelectInventorySlotEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <Factory/ItemFactory.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PlantFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundChannel.hpp>
#include <Systems/InventorySystem.hpp>
#include <Systems/ItemSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/Cardinal.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/System/Time.hpp>
#include <cmath>
#include <spdlog/spdlog.h>

namespace Game::Sys
{

InventorySystem::InventorySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  // The entt::dispatcher is independent of the registry, so it is safe to bind event handlers in
  // the constructor
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&InventorySystem::on_player_action>( this );
  std::ignore = get_systems_event_queue().sink<Events::AddInventoryEvent>().connect<&InventorySystem::on_add_inventory_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::DropInventoryEvent>().connect<&InventorySystem::on_drop_inventory_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::PickupWorldItemEvent>().connect<&InventorySystem::on_pickup_world_item_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::UpdateDmgEvent>().connect<&InventorySystem::on_update_dmg_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::SelectInventorySlotEvent>().connect<&InventorySystem::on_select_inventory_slot_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::MoveInventorySlotEvent>().connect<&InventorySystem::on_move_inventory_slot_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::DropInventorySlotEvent>().connect<&InventorySystem::on_drop_inventory_slot_event>( this );
  SPDLOG_DEBUG( "InventorySystem initialized" );
}

void InventorySystem::update( sf::Time dt )
{
  m_swap_item_cooldown_timer += dt;
  Factory::Player::ensure_inventory_slots( reg() );
  Factory::Particle::delete_expired_particle_sprites( reg(), "player.forage.particle.eating" );

  auto player_entt = Utils::Player::get_entity( reg() );
  if ( reg().any_of<Cmp::Player::ConsumeTimer>( player_entt ) ) { consume_inventory( dt ); }

  update_item_expiry_damage( dt );
  update_world_drop_blocked();

  if ( Utils::Player::is_elderflute_active( reg() ) ) { play_elder_flute_music(); }
  else { stop_elder_flute_music(); }
}

void InventorySystem::on_player_action( const Events::PlayerActionEvent &event )
{
  if ( event.action == Events::PlayerActionEvent::GameActions::SWAP_INVENTORY ) { swap_inventory( /*whole_stack=*/false ); }
  else if ( event.action == Events::PlayerActionEvent::GameActions::SWAP_INVENTORY_ALL ) { swap_inventory( /*whole_stack=*/true ); }
  else if ( event.action == Events::PlayerActionEvent::GameActions::CONSUME ) { try_consume_item(); }
}

void InventorySystem::on_add_inventory_event( Events::AddInventoryEvent ev ) { add_inventory_item( ev.m_item ); }

void InventorySystem::on_drop_inventory_event( [[maybe_unused]] Events::DropInventoryEvent ev )
{
  auto [inventory_entt, _, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_entt == entt::null ) return;
  auto player_pos = Utils::Player::get_position( reg() ).position;
  drop_inventory_item( player_pos, inventory_entt );
}

void InventorySystem::on_pickup_world_item_event( Events::PickupWorldItemEvent ev )
{
  if ( not ev.to_empty_slot )
  {
    pickup_world_item( reg(), ev.world_item_entt );
    return;
  }

  // queued from the drop, so the item may have gone (e.g. picked up or destroyed) and the slots may have filled since
  if ( not reg().valid( ev.world_item_entt ) or not reg().all_of<Cmp::WorldItem>( ev.world_item_entt ) ) return;
  // a stackable item joins a stack it matches first, then any leftover takes an empty slot
  if ( const auto stack_slot_entt = Utils::Player::find_stackable_slot( reg(), ev.world_item_entt ); stack_slot_entt != entt::null )
  {
    merge_into_stack( stack_slot_entt, ev.world_item_entt );
    if ( not reg().valid( ev.world_item_entt ) ) return;
  }
  const auto slot_idx = Utils::Player::get_first_empty_slot_idx( reg() );
  if ( not slot_idx ) return; // every slot is full, so it stays on the ground
  pickup_world_item( reg(), ev.world_item_entt, slot_idx );
}

void InventorySystem::on_select_inventory_slot_event( const Events::SelectInventorySlotEvent &ev )
{
  // can't change hands mid-bite, or let go of the broom mid-flight (including the descent to the landing cell)
  if ( Utils::Player::is_player_eating( reg() ) ) return;
  if ( Utils::Player::is_flying( reg() ) ) return;

  auto player_entt = Utils::Player::get_entity( reg() );
  auto *selected_cmp = reg().try_get<Cmp::Player::SelectedInventorySlot>( player_entt );
  if ( not selected_cmp ) return;

  const uint8_t count = Utils::Player::get_inventory_slot_count( reg() );
  const uint8_t current = Utils::Player::get_selected_slot_idx( reg() );
  const bool rollover = Sys::PersistSystem::get<Cmp::Persist::InventorySlotRollover>( reg() ).get_value();
  uint8_t next = current;
  switch ( ev.m_mode )
  {
    case Events::SelectInventorySlotEvent::Mode::INDEX:
      if ( ev.m_idx >= count ) return;
      next = ev.m_idx;
      break;
    case Events::SelectInventorySlotEvent::Mode::NEXT:
      if ( rollover ) { next = static_cast<uint8_t>( ( current + 1 ) % count ); }
      else if ( current + 1 < count ) { next = static_cast<uint8_t>( current + 1 ); }
      break;
    case Events::SelectInventorySlotEvent::Mode::PREV:
      if ( rollover ) { next = static_cast<uint8_t>( ( current + count - 1 ) % count ); }
      else if ( current > 0 ) { next = static_cast<uint8_t>( current - 1 ); }
      break;
  }
  if ( next == current )
  {
    // keep the stored index in range even if it was clamped by a lower slot count
    selected_cmp->m_idx = current;
    return;
  }

  // a drawn bow (or charged burr) belongs to the item being put away
  get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::CANCEL_BOW ) );
  selected_cmp->m_idx = next;
  SPDLOG_DEBUG( "Selected inventory slot {}", next );
}

void InventorySystem::on_move_inventory_slot_event( const Events::MoveInventorySlotEvent &ev )
{
  // same rules as changing the selected slot: the held item can't be moved mid-bite or mid-flight
  if ( Utils::Player::is_player_eating( reg() ) ) return;
  if ( Utils::Player::is_flying( reg() ) ) return;
  if ( ev.m_from == ev.m_to ) return;

  const uint8_t count = Utils::Player::get_inventory_slot_count( reg() );
  if ( ev.m_from >= count or ev.m_to >= count ) return;
  const auto from_entt = Utils::Player::get_inventory_slot_entt( reg(), ev.m_from );
  const auto to_entt = Utils::Player::get_inventory_slot_entt( reg(), ev.m_to );
  if ( from_entt == entt::null or to_entt == entt::null ) return;

  auto player_entt = Utils::Player::get_entity( reg() );
  auto *selected_cmp = reg().try_get<Cmp::Player::SelectedInventorySlot>( player_entt );
  const uint8_t selected = Utils::Player::get_selected_slot_idx( reg() );
  const bool moves_selected = selected == ev.m_from or selected == ev.m_to;
  // a drawn bow (or charged burr) belongs to the item being moved
  if ( moves_selected ) { get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::CANCEL_BOW ) ); }

  // a slot's position is only its index, so exchanging indices moves each slot's item with all its per-item components
  std::swap( reg().get<Cmp::PlayerInventorySlot>( from_entt ).m_slot_idx, reg().get<Cmp::PlayerInventorySlot>( to_entt ).m_slot_idx );

  // the selection follows the item, so a moved held item stays in hand
  if ( selected_cmp and moves_selected ) { selected_cmp->m_idx = selected == ev.m_from ? ev.m_to : ev.m_from; }
  SPDLOG_DEBUG( "Moved inventory slot {} to {}", ev.m_from, ev.m_to );
}

void InventorySystem::on_drop_inventory_slot_event( const Events::DropInventorySlotEvent &ev )
{
  // same rules as moving a slot: the held item can't be let go of mid-bite or mid-flight
  if ( Utils::Player::is_player_eating( reg() ) ) return;
  if ( Utils::Player::is_flying( reg() ) ) return;
  if ( ev.m_slot >= Utils::Player::get_inventory_slot_count( reg() ) ) return;
  const auto slot_entt = Utils::Player::get_inventory_slot_entt( reg(), ev.m_slot );
  if ( slot_entt == entt::null or reg().get<Cmp::PlayerInventorySlot>( slot_entt ).empty() ) return;

  const auto drop_pos = find_world_drop_cell( slot_entt );
  if ( not drop_pos )
  {
    SPDLOG_DEBUG( "Nowhere to drop inventory slot {}", ev.m_slot );
    return;
  }

  // a drawn bow (or charged burr) belongs to the item being dropped
  if ( ev.m_slot == Utils::Player::get_selected_slot_idx( reg() ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::CANCEL_BOW ) );
  }
  drop_inventory_item( *drop_pos, slot_entt );
}

std::optional<sf::Vector2f> InventorySystem::find_world_drop_cell( entt::entity slot_entt )
{
  const auto &slot_cmp = reg().get<Cmp::PlayerInventorySlot>( slot_entt );
  if ( slot_cmp.empty() ) return std::nullopt;
  const Cmp::Position player_pos = Utils::Player::get_position( reg() );

  // a plant replants at the player's feet, as drop_inventory_item() does for any drop, and never in the spawn area
  if ( slot_cmp.m_item->sprite_type.contains( "plant" ) )
  {
    if ( Utils::Player::is_in_spawn( reg(), player_pos ) ) return std::nullopt;
    return player_pos.position;
  }

  auto npc_navmesh = m_npc_navmesh.lock();
  auto ghost_navmesh = m_ghost_navmesh.lock();
  auto loot_sm = m_loot_sm.lock();
  auto reserved_sm = m_reserved_sm.lock();
  auto item_sm = m_item_sm.lock();
  const Utils::SpawnGrids grids{ .navmesh = npc_navmesh.get(),
                                 .ghost_navmesh = ghost_navmesh.get(),
                                 .loot_sm = loot_sm.get(),
                                 .reserved_sm = reserved_sm.get(),
                                 .item_sm = item_sm.get() };
  return Utils::find_nearest_cell_near( player_pos, /*max_rings=*/1, [&]( const sf::Vector2f &cell_pos )
  { return Utils::is_free_spawn_area( reg(), sf::FloatRect( cell_pos, Constants::kGridSizePxF ), grids ); } );
}

void InventorySystem::update_world_drop_blocked()
{
  auto *drag_ctx = reg().ctx().find<Cmp::InventoryDrag>();
  if ( not drag_ctx ) return;
  drag_ctx->m_world_drop_blocked = false;
  if ( not drag_ctx->m_from_slot ) return;
  const auto slot_entt = Utils::Player::get_inventory_slot_entt( reg(), *drag_ctx->m_from_slot );
  if ( slot_entt == entt::null ) return;
  drag_ctx->m_world_drop_blocked = not find_world_drop_cell( slot_entt ).has_value();
}

entt::entity InventorySystem::prepare_empty_slot( std::optional<uint8_t> idx )
{
  const uint8_t slot_idx = idx.value_or( Utils::Player::get_selected_slot_idx( reg() ) );
  auto slot_entt = Utils::Player::get_inventory_slot_entt( reg(), slot_idx );
  if ( slot_entt == entt::null )
  {
    Factory::Player::ensure_inventory_slots( reg() );
    slot_entt = Utils::Player::get_inventory_slot_entt( reg(), slot_idx );
    if ( slot_entt == entt::null )
    {
      SPDLOG_WARN( "No inventory slot {}", slot_idx );
      return entt::null;
    }
  }

  if ( not reg().get<Cmp::PlayerInventorySlot>( slot_entt ).empty() )
  {
    // callers are expected to free the slot first; drop the occupant rather than lose it
    SPDLOG_WARN( "Inventory slot {} is occupied, dropping its item", slot_idx );
    drop_inventory_item( Utils::Player::get_position( reg() ).position, slot_entt );
    // e.g. a plant can't be replanted in the spawn area
    if ( not reg().get<Cmp::PlayerInventorySlot>( slot_entt ).empty() ) { Factory::Player::clear_inventory_slot( reg(), slot_entt ); }
  }
  return slot_entt;
}

void InventorySystem::on_update_dmg_event( const Events::UpdateDmgEvent &ev ) { update_dmg( ev.m_item_entt, ev.m_amount, ev.m_type ); }

void InventorySystem::update_dmg( entt::entity entt, float amount, Events::UpdateDmgEvent::Type type )
{
  // don't assume the slot has a wear level
  if ( reg().all_of<Cmp::PlayerInventorySlot, Cmp::Inventory::WearLevel>( entt ) )
  {
    auto &wearlevel_cmp = reg().get<Cmp::Inventory::WearLevel>( entt );
    switch ( type )
    {
      case Events::UpdateDmgEvent::ADD:
        wearlevel_cmp.m_level = std::clamp( wearlevel_cmp.m_level + amount, 0.f, 100.f );
        break;
      case Events::UpdateDmgEvent::SUBTRACT:
        wearlevel_cmp.m_level = std::clamp( wearlevel_cmp.m_level - amount, 0.f, 100.f );
        break;
    }

    const auto &slot_cmp = reg().get<Cmp::PlayerInventorySlot>( entt );
    if ( wearlevel_cmp.m_level <= 0 and not slot_cmp.empty() )
    {
      const Sys::ItemKey inventory_type = slot_cmp.m_item->item_type;
      const uint8_t slot_idx = slot_cmp.m_slot_idx;
      auto item = Sys::ItemStore::instance().get( inventory_type );
      if ( item.consumable_type() == Cmp::WorldItem::Consumable::EDIBLE )
      {
        // rotten food takes the place of the food, in the same slot. A stack rots together into a stack of the same size.
        const int stack_count = Utils::Player::get_stack_count( reg(), entt );
        Factory::Player::clear_inventory_slot( reg(), entt );
        add_inventory_item( "item.rottenfood", slot_idx );
        const auto rotten_slot_entt = Utils::Player::get_inventory_slot_entt( reg(), slot_idx );
        if ( stack_count > 1 and rotten_slot_entt != entt::null )
        {
          reg().emplace_or_replace<Cmp::Inventory::StackCount>( rotten_slot_entt, stack_count );
        }
      }
      else if ( inventory_type == "item.gloves" )
      {
        // worn out gloves fall apart
        Factory::Player::clear_inventory_slot( reg(), entt );
        m_sound_bank.get_effect( "drop_inventory" ).play();
      }
      else
      {
        // dead tools remain in inventory for altar offering
        m_sound_bank.get_effect( "tool_break" ).play();
      }
    }
  }
}

void InventorySystem::add_inventory_item( const Sys::ItemKey &item, std::optional<uint8_t> idx )
{
  const auto &item_cmp = Sys::ItemStore::instance().get( item );

  auto inventory_entt = prepare_empty_slot( idx );
  if ( inventory_entt == entt::null ) return;
  reg().get<Cmp::PlayerInventorySlot>( inventory_entt ).m_item = item_cmp;
  if ( item_cmp.has_wear() ) { reg().emplace_or_replace<Cmp::Inventory::WearLevel>( inventory_entt, 100.f ); }
  if ( item_cmp.stings() ) { reg().emplace_or_replace<Cmp::Inventory::StingDamage>( inventory_entt ); }

  if ( item.contains( "torch" ) ) { reg().emplace_or_replace<Cmp::UUID>( inventory_entt, Cmp::UUID::generate() ); }

  const size_t sprite_idx = item_cmp.sprite_index();
  if ( item.contains( "cursetablet" ) ) { Factory::Item::add_curse_tablet( reg(), inventory_entt, item, sprite_idx ); }

  // clang-format off
  reg().emplace_or_replace<Cmp::AnimData>( inventory_entt, Cmp::AnimData::Config{ 
        .sprite_type = item_cmp.sprite_type, 
        .frame_index_offset = sprite_idx,
        .enabled = true
  });
  // clang-format on
}

void InventorySystem::swap_inventory( bool whole_stack )
{
  // if player is standing next to a Cmp::Crypt::Chest let them open it without dropping the inventory item
  for ( auto [chest_entt, chest_cmp, chest_pos_cmp] : reg().view<Cmp::Crypt::Chest, Cmp::Position>().each() )
  {
    if ( not Utils::Player::is_player_near( reg(), chest_pos_cmp ) ) continue;
    return;
  }

  // if player is standing next to a Cmp::Crypt::ObjectiveMultiblock let them activate it without dropping the inventory item.
  // Once activated it has nothing left to give, so E picks up the preserved cat it dropped beside it
  for ( auto [objective_entt, objective_mb_cmp] : reg().view<Cmp::Crypt::ObjectiveMultiBlock>().each() )
  {
    if ( objective_mb_cmp.get_activation_count() >= objective_mb_cmp.get_activation_threshold() ) continue;
    if ( not Utils::Player::is_player_near( reg(), objective_mb_cmp ) ) continue;
    return;
  }

  // if player is facing a Cmp::Altar::MultiBlock let them make an offering (blood or item) without dropping the inventory item
  for ( auto [altar_entt, altar_cmp] : reg().view<Cmp::Altar::MultiBlock>().each() )
  {
    if ( not Utils::Player::is_player_near( reg(), altar_cmp ) ) continue;
    if ( not Utils::Player::get_projected_position( reg() ).findIntersection( altar_cmp ) ) continue;
    return;
  }

  // if player is facing a Cmp::Cart::MultiBlock, E transfers to its barrels (see CartHorseSystem) without dropping the inventory item
  for ( auto [cart_entt, cart_cmp] : reg().view<Cmp::Cart::MultiBlock>().each() )
  {
    if ( not Utils::Player::is_player_near( reg(), cart_cmp ) ) continue;
    if ( not Utils::Player::get_projected_position( reg() ).findIntersection( cart_cmp ) ) continue;
    return;
  }

  // if player is facing a Cmp::Horse::MultiBlock, E opens the leave prompt (see CartHorseSystem) without dropping the inventory item
  for ( auto [horse_entt, horse_cmp] : reg().view<Cmp::Horse::MultiBlock>().each() )
  {
    if ( not Utils::Player::is_player_near( reg(), horse_cmp ) ) continue;
    if ( not Utils::Player::get_projected_position( reg() ).findIntersection( horse_cmp ) ) continue;
    return;
  }

  // still chewing?
  auto *eating_time = Utils::Player::is_player_eating( reg() );
  if ( eating_time ) return;

  // can't let go of the broom mid-flight (including the descent to the landing cell)
  if ( Utils::Player::is_flying( reg() ) ) return;

  const sf::Time swap_cooldown = sf::seconds( Sys::PersistSystem::get<Cmp::Persist::InventorySwapCooldown>( reg() ).get_value() );
  if ( m_swap_item_cooldown_timer < swap_cooldown ) return;

  auto player_pos = Cmp::RectBounds::scaled( Utils::Player::get_position( reg() ), 0.5 );

  // standing on the held stack's item: it joins the stack as far as it fits (see merge_into_stack()), the rest stays
  // on the ground. Nothing is dropped, even when the stack is already full
  if ( const auto selected_entt = Utils::Player::get_selected_slot_entt( reg() ); selected_entt != entt::null )
  {
    auto world_item_view = reg().view<Cmp::WorldItem, Cmp::Position>( entt::exclude<Cmp::PlantMultiBlock> );
    for ( auto [world_item_entt, world_item_cmp, world_item_pos_cmp] : world_item_view.each() )
    {
      if ( not player_pos.findIntersection( world_item_pos_cmp ) ) continue;
      if ( not Utils::Player::is_stack_match( reg(), selected_entt, world_item_entt ) ) continue;
      merge_into_stack( selected_entt, world_item_entt );
      m_swap_item_cooldown_timer = sf::Time::Zero;
      return;
    }
  }

  Sys::SpriteKey existing_player_inventory_type;

  // drop one of the selected slot's stack, or all of it, if it has an item
  if ( auto [inventory_entt, _, inventory_sprite_type] = Utils::Player::get_inventory( reg() ); inventory_entt != entt::null )
  {
    existing_player_inventory_type = inventory_sprite_type;
    const int count = whole_stack ? Utils::Player::get_stack_count( reg(), inventory_entt ) : 1;
    drop_from_slot( Utils::Player::get_position( reg() ).position, inventory_entt, count );
  }

  // pickup inventory into the selected slot if it is now empty and there is something at this position
  // (a plant can't be replanted in the spawn area, so the slot may still be full)
  const auto selected_entt = Utils::Player::get_selected_slot_entt( reg() );
  if ( selected_entt != entt::null and reg().get<Cmp::PlayerInventorySlot>( selected_entt ).empty() )
  {
    auto world_item_view = reg().view<Cmp::WorldItem, Cmp::Position>( entt::exclude<Cmp::PlantMultiBlock> );
    for ( auto [world_item_entt, world_item_cmp, world_item_pos_cmp] : world_item_view.each() )
    {
      if ( not player_pos.findIntersection( world_item_pos_cmp ) ) continue;        // is there something to pick up?
      if ( world_item_cmp.sprite_type == existing_player_inventory_type ) continue; // dont pick up the one we just dropped

      pickup_world_item( reg(), world_item_entt );
      break; // the slot is full now
    }
  }
  m_swap_item_cooldown_timer = sf::Time::Zero;
}

entt::entity InventorySystem::create_world_item_from_slot( sf::Vector2f pos, entt::entity inventory_slot_entt )
{
  const auto &inventory_slot_cmp = reg().get<Cmp::PlayerInventorySlot>( inventory_slot_entt );
  auto world_item_entt = reg().create();
  reg().emplace_or_replace<Cmp::Position>( world_item_entt, pos, Constants::kGridSizePxF );

  // Copy the animdata over to avoid losing ref data
  auto *existing_anim_data = reg().try_get<Cmp::AnimData>( inventory_slot_entt );
  if ( existing_anim_data ) { reg().emplace_or_replace<Cmp::AnimData>( world_item_entt, *existing_anim_data ); }
  else { SPDLOG_WARN( "Dropped item {} has no AnimData", inventory_slot_cmp.m_item->item_type.str() ); }

  reg().emplace_or_replace<Cmp::ZOrderValue>( world_item_entt, pos.y - 1.f );
  reg().emplace_or_replace<Cmp::WorldItem>( world_item_entt, *inventory_slot_cmp.m_item );
  reg().emplace_or_replace<Cmp::Npc::NoPathFinding>( world_item_entt );
  if ( auto item_sm = m_item_sm.lock() ) item_sm->insert( world_item_entt, reg().get<Cmp::Position>( world_item_entt ) );

  // try to copy any relevant components over to the new world carryitem entt
  auto *inventory_slot_level_cmp = reg().try_get<Cmp::Inventory::WearLevel>( inventory_slot_entt );
  if ( inventory_slot_level_cmp ) { reg().emplace_or_replace<Cmp::Inventory::WearLevel>( world_item_entt, inventory_slot_level_cmp->m_level ); }
  if ( reg().all_of<Cmp::Inventory::StingDamage>( inventory_slot_entt ) )
  {
    reg().emplace_or_replace<Cmp::Inventory::StingDamage>( world_item_entt );
  }
  return world_item_entt;
}

void InventorySystem::drop_from_slot( sf::Vector2f pos, entt::entity inventory_slot_entt, int count )
{
  const auto *inventory_slot_cmp = reg().try_get<Cmp::PlayerInventorySlot>( inventory_slot_entt );
  if ( not inventory_slot_cmp or inventory_slot_cmp->empty() or count <= 0 ) return;
  // the whole slot drops as it is, part of its stack is split off
  if ( count >= Utils::Player::get_stack_count( reg(), inventory_slot_entt ) ) { drop_inventory_item( pos, inventory_slot_entt ); }
  else { split_from_slot( pos, inventory_slot_entt, count ); }
}

void InventorySystem::split_from_slot( sf::Vector2f pos, entt::entity inventory_slot_entt, int count )
{
  const int slot_count = Utils::Player::get_stack_count( reg(), inventory_slot_entt );
  if ( count <= 0 or count >= slot_count ) return;

  // stackable items carry no per-instance state (see Utils::Player::can_stack()), so only the shared components are copied
  const auto world_item_entt = create_world_item_from_slot( pos, inventory_slot_entt );
  if ( count > 1 ) { reg().emplace_or_replace<Cmp::Inventory::StackCount>( world_item_entt, count ); }
  reg().emplace_or_replace<Cmp::Inventory::StackCount>( inventory_slot_entt, slot_count - count );

  // the slot keeps its own UUID (and a torch's flame), so the split off item gets a new one, and a torch its own flame
  reg().emplace_or_replace<Cmp::UUID>( world_item_entt, Cmp::UUID::generate() );
  Factory::Particle::add_flame_for_world_item( reg(), world_item_entt, Factory::Particle::kWorldTorchFlameTag );

  SPDLOG_DEBUG( "Dropped {} x{} from inventory stack (now {})", reg().get<Cmp::WorldItem>( world_item_entt ).item_type.str(), count,
                slot_count - count );
  m_sound_bank.get_effect( "drop_inventory" ).play();
}

void InventorySystem::drop_inventory_item( sf::Vector2f pos, entt::entity inventory_slot_entt )
{
  auto *inventory_slot_cmp = reg().try_get<Cmp::PlayerInventorySlot>( inventory_slot_entt );

  if ( not inventory_slot_cmp or inventory_slot_cmp->empty() )
  {
    SPDLOG_INFO( "Player has no inventory" );
    return;
  }

  // if player inventory is a plant item then replant it in the ground - snap to nearest grid to prevent collision issues
  if ( inventory_slot_cmp->m_item->sprite_type.contains( "plant" ) )
  {
    // multiblocks are top-left anchored, so offset the y-axis so that plant base is at players feet
    auto plant_pos = Utils::snap_to_grid( { pos.x, pos.y - Constants::kGridSizePxF.y } );

    // don't allow plants to be dropped at all in the player spawn area
    if ( Utils::Player::is_in_spawn( reg(), Utils::Player::get_position( reg() ) ) ) return;

    const auto &spritesheet = Sys::SpriteStore::instance().get( inventory_slot_cmp->m_item->sprite_type );
    // keep the frame the item already has (e.g. rolled by sprite.random when it first spawned)
    const auto *slot_anim_cmp = reg().try_get<Cmp::AnimData>( inventory_slot_entt );
    const std::size_t sprite_idx = slot_anim_cmp ? slot_anim_cmp->getFrameIndexOffset() : inventory_slot_cmp->m_item->sprite_index();
    // grass doesn't reserve its cell, so clear any under the new plant's footprint
    Factory::Plant::destroy_grass_in( reg(), sf::FloatRect( plant_pos, spritesheet.sprite_size() ) );
    // a new, unrotated multiblock: a plant dropped on its side (see PlantSystem::on_plant_killed_event) stands upright again
    auto [mb_entt, segment_entt_list] = Factory::Multiblock::add_multiblock_with_segments<Cmp::PlantMultiBlock, Cmp::PlantSegment>(
        reg(), plant_pos, spritesheet, /*ss_index=*/sprite_idx, /*zorder=*/0, m_reserved_sm.lock().get() );

    // Preserve the item this plant was grown from, so digging it back up (see the DIG handler in
    // on_player_action_event) can hand it back via the normal pickup_world_item path instead of
    // having to re-derive an item id from the multiblock's sprite.
    reg().emplace_or_replace<Cmp::WorldItem>( mb_entt, *inventory_slot_cmp->m_item );
    // a carried plant keeps its health as the slot's wear level, so it is replanted with the health it was dug up with
    const auto *plant_wearlevel_cmp = reg().try_get<Cmp::Inventory::WearLevel>( inventory_slot_entt );
    reg().emplace_or_replace<Cmp::Health>( mb_entt, plant_wearlevel_cmp ? plant_wearlevel_cmp->m_level : 100.f );
    if ( reg().all_of<Cmp::Inventory::StingDamage>( inventory_slot_entt ) ) { reg().emplace_or_replace<Cmp::Inventory::StingDamage>( mb_entt ); }

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
        auto *seg_pos_cmp = reg().try_get<Cmp::Position>( seg_entt );
        if ( not seg_pos_cmp ) continue;
        for ( auto blocked_entt : npc_navmesh->at( *seg_pos_cmp ) )
        {
          if ( reg().any_of<Cmp::Player::Character>( blocked_entt ) ) continue;
          npc_navmesh->remove( blocked_entt, *seg_pos_cmp );
        }
      }
    }

    // clear player inventory slot
    Factory::Player::clear_inventory_slot( reg(), inventory_slot_entt );
    m_sound_bank.get_effect( "chopping_final" ).play();
    return;
  }

  // otherwise just drop it as a world item
  const auto world_item_entt = create_world_item_from_slot( pos, inventory_slot_entt );
  // the whole stack drops as one world item
  if ( const auto *stack_cmp = reg().try_get<Cmp::Inventory::StackCount>( inventory_slot_entt ) )
  {
    reg().emplace_or_replace<Cmp::Inventory::StackCount>( world_item_entt, stack_cmp->m_count );
  }

  if ( const auto *offering_cmp = reg().try_get<Cmp::Npc::SummonOffering>( inventory_slot_entt ) )
  {
    reg().emplace_or_replace<Cmp::Npc::SummonOffering>( world_item_entt, Cmp::Npc::SummonOffering{ offering_cmp->m_summoner } );
  }

  if ( const auto *curse_tablet_cmp = reg().try_get<Cmp::Grave::CurseTablet>( inventory_slot_entt ) )
  {
    reg().emplace_or_replace<Cmp::Grave::CurseTablet>( world_item_entt, *curse_tablet_cmp );
  }

  auto *inventory_slot_dowsing_cmp = reg().try_get<Cmp::Inventory::DowsingTarget>( inventory_slot_entt );
  if ( inventory_slot_dowsing_cmp )
  {
    reg().emplace_or_replace<Cmp::Inventory::DowsingTarget>( world_item_entt, inventory_slot_dowsing_cmp->target );
  }

  auto *uuid_cmp = reg().try_get<Cmp::UUID>( inventory_slot_entt );
  if ( uuid_cmp )
  {

    for ( auto [ps_entt, ps_owner, ps_uuid_cmp] : reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
    {
      if ( ps_uuid_cmp == *uuid_cmp )
      {
        ps_owner.sprite->set_view_type( Cmp::Particle::ViewType::WORLD );
        ps_owner.sprite->set_scale( Cmp::Particle::kWorldScalePreset );
        // one above the dropped item zorder so the flame never ties with its own torch
        reg().emplace_or_replace<Cmp::ZOrderValue>( ps_entt, pos.y );
      }
    }
    reg().emplace_or_replace<Cmp::UUID>( world_item_entt, uuid_cmp->data );
  }

  // now empty the inventory slot
  Factory::Player::clear_inventory_slot( reg(), inventory_slot_entt );
  if ( world_item_entt != entt::null ) { m_sound_bank.get_effect( "drop_inventory" ).play(); }
}

void InventorySystem::update_item_expiry_damage( sf::Time dt )
{
  static constexpr sf::Time expiry_update_timeout = sf::seconds( 1.f );
  m_expiry_update_timer += dt;
  if ( m_expiry_update_timer < expiry_update_timeout ) return;
  m_expiry_update_timer = sf::Time::Zero;

  // Collect world entities first - don't modify the registry while iterating its views
  std::vector<entt::entity> world_items;
  for ( auto [worlditem_entt, worlditem_cmp] : reg().view<Cmp::WorldItem>().each() )
  {
    if ( worlditem_cmp.world_expiry() == sf::Time::Zero ) continue;
    // plants decay through their health, everything else through its wear level
    if ( not reg().any_of<Cmp::Inventory::WearLevel, Cmp::Health>( worlditem_entt ) ) continue;
    world_items.push_back( worlditem_entt );
  }
  for ( auto worlditem_entt : world_items )
  {
    auto &worlditem_cmp = reg().get<Cmp::WorldItem>( worlditem_entt );
    float dmg = 100.f / ( worlditem_cmp.world_expiry().asSeconds() / expiry_update_timeout.asSeconds() );
    get_systems_event_queue().trigger( Events::UpdateDmgEvent( worlditem_entt, dmg ) );
  }

  // every slot rots, selected or not. Collect first - update_dmg may empty and refill a slot.
  const uint8_t selected_idx = Utils::Player::get_selected_slot_idx( reg() );
  std::vector<std::pair<entt::entity, float>> inventory_dmg;
  for ( auto [inventory_entt, inventory_cmp, wearlevel_cmp] : reg().view<Cmp::PlayerInventorySlot, Cmp::Inventory::WearLevel>().each() )
  {
    if ( inventory_cmp.empty() or wearlevel_cmp.m_level <= 0 ) continue;
    // "active_only" items (e.g. elderflute, witchesbroom) only wear while they are selected and in use
    if ( inventory_cmp.m_item->wears_only_when_active() and
         ( inventory_cmp.m_slot_idx != selected_idx or not Utils::Player::is_inventory_item_active( reg() ) ) )
    {
      continue;
    }
    // the slot entity has no Cmp::WorldItem of its own - the item lives inside the slot component.
    // Tools have no expiry; they only wear through use.
    if ( inventory_cmp.m_item->inventory_expiry() != sf::Time::Zero )
    {
      float dmg = 100.f / ( inventory_cmp.m_item->inventory_expiry().asSeconds() / expiry_update_timeout.asSeconds() );
      inventory_dmg.emplace_back( inventory_entt, dmg );
    }
  }
  for ( auto [inventory_entt, dmg] : inventory_dmg )
  {
    update_dmg( inventory_entt, dmg, Events::UpdateDmgEvent::SUBTRACT );
  }
}

void InventorySystem::pickup_world_item( entt::registry &reg, entt::entity world_item_entt, std::optional<uint8_t> idx )
{

  // items without a Cmp::Inventory::StingDamage (e.g. foraged with gloves) don't sting when picked up
  const auto spawn_action = Utils::Player::apply_action_from_world_item<Cmp::SpawnAction>( reg, world_item_entt );

  //! @todo we dont want to automatically play the spawn sfx every time we pick up an item. Maybe this should be a toggle in the json?
  // play_action_sfx( spawn_action );

  if ( const auto *world_item_pos_cmp = reg.try_get<Cmp::Position>( world_item_entt ) )
  {
    trigger_side_effects( spawn_action, *world_item_pos_cmp, world_item_entt );
  }

  auto *world_item_cmp = reg.try_get<Cmp::WorldItem>( world_item_entt );
  if ( not world_item_cmp )
    throw std::runtime_error( "InventorySystem::pickup_world_item - Unable to get world item component from " +
                              std::to_string( static_cast<uint32_t>( world_item_entt ) ) );

  // copy the item key now, the world item entt is destroyed below
  const Sys::ItemKey picked_up_item = world_item_cmp->item_type;

  // fill the (empty) inventory slot in place, so it keeps its position
  auto inventory_entity = prepare_empty_slot( idx );
  if ( inventory_entity == entt::null ) return;
  // prepare_empty_slot may have dropped an item into the world, so look the world item up again
  world_item_cmp = reg.try_get<Cmp::WorldItem>( world_item_entt );
  if ( not world_item_cmp ) return;
  reg.get<Cmp::PlayerInventorySlot>( inventory_entity ).m_item = *world_item_cmp;

  // Copy the animdata over to avoid losing ref data
  auto *existing_anim_data = reg.try_get<Cmp::AnimData>( world_item_entt );
  if ( existing_anim_data ) { reg.emplace_or_replace<Cmp::AnimData>( inventory_entity, *existing_anim_data ); }

  // If the worlditem has a particle sprite set the scale, zorder and view type.
  auto *uuid_cmp = reg.try_get<Cmp::UUID>( world_item_entt );
  if ( uuid_cmp )
  {
    for ( auto [ps_entt, ps_owner, ps_uuid_cmp] : reg.view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
    {
      if ( ps_uuid_cmp != *uuid_cmp ) continue;
      ps_owner.sprite->clear();
      ps_owner.sprite->set_scale( Cmp::Particle::kUiScalePreset );
      ps_owner.sprite->set_view_type( Cmp::Particle::ViewType::SCREEN );
      reg.emplace_or_replace<Cmp::ZOrderValue>( ps_entt, 50000 );
    }
    reg.emplace_or_replace<Cmp::UUID>( inventory_entity, uuid_cmp->data );
  }

  // transfer the dowsing target from the worlditem to the new player inventory item. The slot has no position,
  // so give it a fixed zorder to draw its guide lines just above the floor and below everything else.
  auto *dowsing_target_cmp = reg.try_get<Cmp::Inventory::DowsingTarget>( world_item_entt );
  if ( dowsing_target_cmp )
  {
    reg.emplace_or_replace<Cmp::Inventory::DowsingTarget>( inventory_entity, dowsing_target_cmp->target );
    reg.emplace_or_replace<Cmp::ZOrderValue>( inventory_entity, -12.f );
  }

  // keep a summoning offering bound to its summoned NPC while it is carried
  if ( const auto *offering_cmp = reg.try_get<Cmp::Npc::SummonOffering>( world_item_entt ) )
  {
    reg.emplace_or_replace<Cmp::Npc::SummonOffering>( inventory_entity, Cmp::Npc::SummonOffering{ offering_cmp->m_summoner } );
  }

  if ( const auto *curse_tablet_cmp = reg.try_get<Cmp::Grave::CurseTablet>( world_item_entt ) )
  {
    reg.emplace_or_replace<Cmp::Grave::CurseTablet>( inventory_entity, *curse_tablet_cmp );
  }

  // transfer the wear level from the worlditem to the new player inventory item. A plant's health is carried as the
  // slot's wear level, if the plant item wears at all
  if ( const auto *wear_level_cmp = reg.try_get<Cmp::Inventory::WearLevel>( world_item_entt ) )
  {
    reg.emplace_or_replace<Cmp::Inventory::WearLevel>( inventory_entity, wear_level_cmp->m_level );
  }
  else if ( const auto *health_cmp = reg.try_get<Cmp::Health>( world_item_entt ); health_cmp and reg.get<Cmp::WorldItem>( world_item_entt ).has_wear() )
  {
    reg.emplace_or_replace<Cmp::Inventory::WearLevel>( inventory_entity, health_cmp->m_level );
  }
  if ( reg.all_of<Cmp::Inventory::StingDamage>( world_item_entt ) ) { reg.emplace_or_replace<Cmp::Inventory::StingDamage>( inventory_entity ); }
  if ( const auto *stack_cmp = reg.try_get<Cmp::Inventory::StackCount>( world_item_entt ) )
  {
    reg.emplace_or_replace<Cmp::Inventory::StackCount>( inventory_entity, stack_cmp->m_count );
  }

  // now destroy the world item entt
  get_systems_event_queue().trigger( Events::DestroyPlantEvent( world_item_entt ) );
  if ( reg.valid( world_item_entt ) )
  {
    if ( auto item_sm = m_item_sm.lock() )
    {
      if ( auto *pos_cmp = reg.try_get<Cmp::Position>( world_item_entt ) ) item_sm->remove( world_item_entt, *pos_cmp );
    }
    reg.destroy( world_item_entt );
  }

  if ( inventory_entity != entt::null )
  {
    m_sound_bank.get_effect( "equip_inventory" ).play();
    // enqueued rather than triggered, as callers may still be iterating registry views
    get_systems_event_queue().enqueue( Events::ItemPickedUpEvent( picked_up_item ) );
  }
}

bool InventorySystem::merge_into_stack( entt::entity slot_entt, entt::entity world_item_entt )
{
  if ( not Utils::Player::can_stack( reg(), slot_entt, world_item_entt ) ) return false;
  const int max_stack = Sys::PersistSystem::get<Cmp::Persist::InventoryStackSize>( reg() ).get_value();
  const int slot_count = Utils::Player::get_stack_count( reg(), slot_entt );
  const int ground_count = Utils::Player::get_stack_count( reg(), world_item_entt );
  const int taken = std::min( max_stack - slot_count, ground_count );
  if ( taken <= 0 ) return false;

  // copy the item key now, the world item entt may be destroyed below
  const Sys::ItemKey picked_up_item = reg().get<Cmp::WorldItem>( world_item_entt ).item_type;

  // the same effects as any other pickup, see pickup_world_item()
  const auto spawn_action = Utils::Player::apply_action_from_world_item<Cmp::SpawnAction>( reg(), world_item_entt );
  if ( const auto *world_item_pos_cmp = reg().try_get<Cmp::Position>( world_item_entt ) )
  {
    trigger_side_effects( spawn_action, *world_item_pos_cmp, world_item_entt );
  }

  // a stack shares one wear level: the count-weighted average of the slot's and the merged items' wear.
  // A side without a wear level counts as unworn. Any leftover on the ground keeps its own wear.
  const auto *slot_wear_cmp = reg().try_get<Cmp::Inventory::WearLevel>( slot_entt );
  const auto *ground_wear_cmp = reg().valid( world_item_entt ) ? reg().try_get<Cmp::Inventory::WearLevel>( world_item_entt ) : nullptr;
  if ( slot_wear_cmp or ground_wear_cmp )
  {
    const float slot_wear = slot_wear_cmp ? slot_wear_cmp->m_level : 100.f;
    const float ground_wear = ground_wear_cmp ? ground_wear_cmp->m_level : 100.f;
    const float avg_wear = ( slot_wear * static_cast<float>( slot_count ) + ground_wear * static_cast<float>( taken ) ) /
                           static_cast<float>( slot_count + taken );
    reg().emplace_or_replace<Cmp::Inventory::WearLevel>( slot_entt, avg_wear );
  }

  reg().emplace_or_replace<Cmp::Inventory::StackCount>( slot_entt, slot_count + taken );
  if ( reg().valid( world_item_entt ) )
  {
    if ( ground_count > taken ) { reg().emplace_or_replace<Cmp::Inventory::StackCount>( world_item_entt, ground_count - taken ); }
    else
    {
      if ( auto item_sm = m_item_sm.lock() )
      {
        if ( auto *pos_cmp = reg().try_get<Cmp::Position>( world_item_entt ) ) item_sm->remove( world_item_entt, *pos_cmp );
      }
      // the stack keeps the slot's particles (e.g. a torch's flame), so put out the merged item's own, or it would be
      // left burning on the ground where the item was
      Factory::Particle::destroy_bound_particles( reg(), world_item_entt );
      reg().destroy( world_item_entt );
    }
  }

  SPDLOG_DEBUG( "Stacked {} x{} into inventory slot (now {})", picked_up_item.str(), taken, slot_count + taken );
  m_sound_bank.get_effect( "equip_inventory" ).play();
  // enqueued rather than triggered, as callers may still be iterating registry views
  get_systems_event_queue().enqueue( Events::ItemPickedUpEvent( picked_up_item ) );
  return true;
}

void InventorySystem::try_consume_item()
{
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type == "" ) return;

  auto item_type = Sys::ItemStore::instance().get( inventory_type );
  if ( item_type.consumable_type() == Cmp::WorldItem::Consumable::NONE ) return;

  auto player_entt = Utils::Player::get_entity( reg() );
  // blood only heals, so it would be wasted at full health
  if ( item_type.is_blood() and Utils::Player::get_stats( reg() ).is_full_health() and not reg().any_of<Cmp::Player::ConsumeTimer>( player_entt ) )
  {
    // signal UI to flash
    reg().emplace_or_replace<Cmp::FlashUIHealth>( reg().create() );
    get_systems_event_queue().trigger( Events::ConsumeRefusedEvent( item_type.item_type ) );
    return;
  }
  if ( not reg().any_of<Cmp::Player::ConsumeTimer>( player_entt ) ) { reg().emplace_or_replace<Cmp::Player::ConsumeTimer>( player_entt ); }
}

void InventorySystem::consume_inventory( sf::Time dt )
{
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type == "" ) return;

  auto item_type = Sys::ItemStore::instance().get( inventory_type );
  if ( item_type.consumable_type() == Cmp::WorldItem::Consumable::NONE ) return;

  auto *consume_timer = Utils::Player::is_player_eating( reg() );
  if ( not consume_timer ) return;

  const auto &consumable_type = item_type.consumable_type();
  // consume_action.sfx: "loop" plays while consuming, "final" once done (see items.json)
  const auto consume_sfx_action = item_type.get_action<Cmp::ConsumeAction>();
  if ( *consume_timer < consume_timer->timeout() )
  {
    *consume_timer += dt;

    // stll consuming
    play_action_loop_sfx( consume_sfx_action );
    if ( consumable_type == Cmp::WorldItem::Consumable::EDIBLE )
    {
      auto uuid = Cmp::UUID::generate();
      auto player_pos = Utils::Player::get_position( reg() ).getCenter();
      auto adj_player_pos = sf::Vector2f( player_pos.x, player_pos.y + 3 );
      auto player_zorder_pos = Utils::Player::get_position( reg() ).position.y;
      constexpr auto kParticleCount = 1;
      constexpr auto kLifetimeSeconds = 1.f;
      constexpr auto kSpeed = 25.f;
      constexpr auto kSize = 5.f;
      const auto &last_direction = Utils::Player::get_last_direction( reg() );
      if ( last_direction == Utils::Cardinal( Utils::Cardinal::North ).vector() )
      {
        Factory::Particle::add_eatingcrumbs_ps( reg(), "player.forage.particle.eating", kParticleCount, kLifetimeSeconds, kSpeed, kSize, uuid,
                                                adj_player_pos, last_direction, player_zorder_pos - 1.f );
      }
      else if ( last_direction == Utils::Cardinal( Utils::Cardinal::East ).vector() )
      {
        Factory::Particle::add_eatingcrumbs_ps( reg(), "player.forage.particle.eating", kParticleCount, kLifetimeSeconds, kSpeed, kSize, uuid,
                                                { adj_player_pos.x + 2, adj_player_pos.y }, last_direction, player_zorder_pos + 1.f );
      }
      else if ( last_direction == Utils::Cardinal( Utils::Cardinal::West ).vector() )
      {
        Factory::Particle::add_eatingcrumbs_ps( reg(), "player.forage.particle.eating", kParticleCount, kLifetimeSeconds, kSpeed, kSize, uuid,
                                                { adj_player_pos.x - 2, adj_player_pos.y }, last_direction, player_zorder_pos + 1.f );
      }
      else if ( last_direction == Utils::Cardinal( Utils::Cardinal::South ).vector() )
      {
        Factory::Particle::add_eatingcrumbs_ps( reg(), "player.forage.particle.eating", kParticleCount, kLifetimeSeconds, kSpeed, kSize, uuid,
                                                adj_player_pos, last_direction, player_zorder_pos + 1.f );
      }
    }
  }
  else
  {
    // all done
    stop_action_loop_sfx( consume_sfx_action );
    play_action_sfx( consume_sfx_action );
    reg().remove<Cmp::Player::ConsumeTimer>( Utils::Player::get_entity( reg() ) );
    const auto consume_action = Utils::Player::apply_action_from_inventory_item<Cmp::ConsumeAction>( reg() );
    trigger_side_effects( consume_action, Utils::Player::get_position( reg() ) );
    // use up one item of the selected slot itself, not another slot holding the same item
    if ( auto inventory_entt = Utils::Player::get_inventory_entt( reg() ); inventory_entt != entt::null )
    {
      Factory::Player::take_one_from_slot( reg(), inventory_entt );
    }
  }
}

void InventorySystem::play_elder_flute_music()
{
  m_sound_bank.get_music( "graveyard_music" ).pause();
  if ( m_sound_bank.get_music( "elder_flute" ).getStatus() != sf::Music::Status::Playing )
  {
    m_sound_bank.get_music( "elder_flute" ).setLooping( true );
    m_sound_bank.get_music( "elder_flute" ).play();
  }
}

void InventorySystem::stop_elder_flute_music()
{
  if ( m_sound_bank.get_music( "elder_flute" ).getStatus() != sf::Music::Status::Playing ) return;
  m_sound_bank.get_music( "elder_flute" ).stop();
  if ( m_sound_bank.get_music( "graveyard_music" ).getStatus() == sf::Music::Status::Paused ) { m_sound_bank.get_music( "graveyard_music" ).play(); }
}

} // namespace Game::Sys
