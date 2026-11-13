#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteAlpha.hpp>
#include <Components/AbsoluteRenderOffset.hpp>
#include <Components/AbsoluteTint.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/AnimData.hpp>
#include <Components/Crypt/BuildingSegment.hpp>
#include <Components/Crypt/ObjectiveSegment.hpp>
#include <Components/DeathPosition.hpp>
#include <Components/Direction.hpp>
#include <Components/FootStepTimer.hpp>
#include <Components/Grave/ExitSegment.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Grave/Segment.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LerpPosition.hpp>
#include <Components/Npc/Adder.hpp>
#include <Components/Npc/Azazel.hpp>
#include <Components/Npc/Burryman.hpp>
#include <Components/Npc/Container.hpp>
#include <Components/Npc/Crow.hpp>
#include <Components/Npc/Drknox.hpp>
#include <Components/Npc/ElderMother.hpp>
#include <Components/Npc/Friendly.hpp>
#include <Components/Npc/Ghost.hpp>
#include <Components/Npc/LerpSpeed.hpp>
#include <Components/Npc/Mimic.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/Priest.hpp>
#include <Components/Npc/ShadowHand.hpp>
#include <Components/Npc/Shockwave.hpp>
#include <Components/Npc/ShockwaveTimer.hpp>
#include <Components/Npc/Skeleton.hpp>
#include <Components/Npc/SpawnedBy.hpp>
#include <Components/Npc/Spider.hpp>
#include <Components/Npc/SummonOffering.hpp>
#include <Components/Npc/Watchman.hpp>
#include <Components/Npc/WatchmanSearchlight.hpp>
#include <Components/Npc/Wisp.hpp>
#include <Components/Npc/Witch.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Particle/EmberParticleSprite.hpp>
#include <Components/Particle/LavaFireballParticleSprite.hpp>
#include <Components/Particle/SpriteOwner.hpp>
#include <Components/PendingTeleport.hpp>
#include <Components/Persistent/AzazelFireballArc.hpp>
#include <Components/Persistent/AzazelFireballColour.hpp>
#include <Components/Persistent/AzazelFireballCooldown.hpp>
#include <Components/Persistent/AzazelFireballSpeed.hpp>
#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Persistent/NpcActivateScale.hpp>
#include <Components/Persistent/NpcRenderOffset.hpp>
#include <Components/Persistent/NpcCrowFlightSpeed.hpp>
#include <Components/Persistent/NpcCrowPerchSpeed.hpp>
#include <Components/Persistent/NpcCrowPerchRange.hpp>
#include <Components/Persistent/NpcCrowMaxPerchWait.hpp>
#include <Components/Persistent/NpcMimicTint.hpp>
#include <Components/Persistent/NpcShockwaveMaxRadius.hpp>
#include <Components/Persistent/NpcShockwaveSpeed.hpp>
#include <Components/Plant/KillNpcTimer.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Player/RepelNpc.hpp>
#include <Components/Player/TookDamage.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Ruin/BuildingSegment.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/SpawnArea.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/CollisionAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ProjectileAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Components/UUID.hpp>
#include <Components/Wall.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/CreateNpcEvent.hpp>
#include <Events/CrowDeliveryEvent.hpp>
#include <Events/DestroyNpcEvent.hpp>
#include <Events/DestroyPlantEvent.hpp>
#include <Events/GrimoireItemUpdateEvent.hpp>
#include <Events/GrimoireNpcUpdateEvent.hpp>
#include <Events/NpcCollisionEvent.hpp>
#include <Events/NpcProjectileEvent.hpp>
#include <Events/NpcProximityEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Events/TeleportEntityEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <Events/UpdateNpcPositionEvent.hpp>
#include <Events/UpdatePlayerPositionEvent.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/FlyingSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Threats/NpcSystem.hpp>
#include <Systems/Threats/ShockwaveSystem.hpp>
#include <Utils/Cardinal.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Npc.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Audio/Sound.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Time.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <spdlog/spdlog.h>
#include <utility>
#include <vector>

namespace Game::Sys
{

NpcSystem::NpcSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  SPDLOG_DEBUG( "NpcSystem initialized" );
  std::ignore = get_systems_event_queue().sink<Events::UpdateNpcPositionEvent>().connect<&NpcSystem::on_update_npc_position_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::DestroyNpcEvent>().connect<&NpcSystem::on_destroy_npc_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::CreateNpcEvent>().connect<&NpcSystem::on_create_npc_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::CrowDeliveryEvent>().connect<&NpcSystem::on_crow_delivery_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::NpcProximityEvent>().connect<&NpcSystem::on_npc_proximity_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::NpcCollisionEvent>().connect<&NpcSystem::on_npc_collision_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::NpcProjectileEvent>().connect<&NpcSystem::on_npc_projectile_event>( this );
}

template <typename ActionT>
Cmp::BaseAction NpcSystem::apply_npc_action( const Sys::NpcKey &npc_type, const Cmp::BaseAction &action )
{
  if ( action.health() < 0 ) { Utils::Player::get_mortality( reg() ).last_damage_cause = Cmp::Player::Mortality::State::HAUNTED; }
  Utils::Player::get_stats( reg() ).apply( action );
  get_systems_event_queue().trigger( Events::GrimoireNpcUpdateEvent( npc_type, ActionT::kKey, action ) );
  play_action_sfx( action );
  return action;
}

void NpcSystem::apply_npc_spawn_action( entt::entity npc_entt )
{
  if ( npc_entt == entt::null ) return;
  const auto &npc_cmp = reg().get<Cmp::Npc::NPC>( npc_entt );
  apply_npc_action<Cmp::SpawnAction>( npc_cmp.npc_type, npc_cmp.at<Cmp::SpawnAction>().action );
}

void NpcSystem::on_npc_proximity_event( const Events::NpcProximityEvent &event )
{
  const auto &action = Sys::NpcStore::instance().get( event.m_npc_type ).at<Cmp::ProximityAction>().action;
  if ( action.health() < 0 ) { reg().emplace_or_replace<Cmp::Player::TookDamage>( Utils::Player::get_entity( reg() ) ); }
  trigger_side_effects( apply_npc_action<Cmp::ProximityAction>( event.m_npc_type, action ), event.m_source );
}

void NpcSystem::on_npc_collision_event( const Events::NpcCollisionEvent &event )
{
  const auto &action = Sys::NpcStore::instance().get( event.m_npc_type ).at<Cmp::CollisionAction>().action;
  trigger_side_effects( apply_npc_action<Cmp::CollisionAction>( event.m_npc_type, action ), event.m_source );
}

void NpcSystem::on_npc_projectile_event( const Events::NpcProjectileEvent &event )
{
  const auto &action = Sys::NpcStore::instance().get( event.m_npc_type ).at<Cmp::ProjectileAction>().action;
  trigger_side_effects( apply_npc_action<Cmp::ProjectileAction>( event.m_npc_type, action ), event.m_source );
}

void NpcSystem::check_npc_proximity( sf::Time dt )
{
  // NPC proximity modifiers are only applied when they are within this rectangle
  auto half_view = Cmp::RectBounds::scaled( Utils::calculate_view_bounds( Sys::RenderSystem::get_world_view() ), 0.5f );

  for ( auto [npc_entt, npc_cmp, npc_pos_cmp, npc_anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::AnimData>().each() )
  {
    auto &[proximity_action, proximity_timer] = npc_cmp.at<Cmp::ProximityAction>();
    if ( proximity_action.interval() == 0.f ) continue;
    // the timer keeps running while the NPC is out of range
    proximity_timer += dt;

    if ( not Utils::is_visible_in_view( half_view.getBounds(), npc_pos_cmp ) ) continue;
    if ( proximity_timer.asSeconds() < proximity_action.interval() ) continue;
    proximity_timer = sf::Time::Zero;
    get_systems_event_queue().trigger( Events::NpcProximityEvent( npc_cmp.npc_type, npc_pos_cmp ) );
  }
}

void NpcSystem::on_create_npc_event( const Events::CreateNpcEvent &event )
{
  SPDLOG_INFO( "Creating NPC {}, max {}", event.m_npc_type.str(), event.m_max_instances );
  if ( event.m_max_instances > 0 and not event.m_source_uuid.empty() )
  {
    if ( Utils::Npc::count_spawned_by( reg(), event.m_source_uuid ) >= event.m_max_instances ) return;
  }

  Cmp::Position spawn_pos = event.m_position;
  if ( event.m_placement == Events::CreateNpcEvent::Placement::NEARBY )
  {
    auto cell = find_nearby_spawn_cell( event.m_position, event.m_npc_type );
    if ( not cell )
    {
      SPDLOG_WARN( "No free cell to spawn {} near [{},{}].", event.m_npc_type.str(), event.m_position.position.x, event.m_position.position.y );
      return;
    }
    spawn_pos = Cmp::Position( *cell, Constants::kGridSizePxF );
  }
  else if ( event.m_placement == Events::CreateNpcEvent::Placement::CENTER )
  {
    // NPCs are top-left anchored, so offset by half the sprite. Deliberately not snapped to the grid.
    const sf::Vector2f npc_size = npc_sprite_size( event.m_npc_type );
    spawn_pos = Cmp::Position( event.m_position.getCenter() - npc_size / 2.f, npc_size );
  }

  auto npc_entt = Factory::Npc::create_npc( reg(), spawn_pos, event.m_npc_type );
  if ( npc_entt == entt::null ) return;
  apply_npc_spawn_action( npc_entt );
  // insert now rather than on its first move, so spawn searches see it straight away
  if ( auto navmesh = navmesh_for( npc_entt ) ) { navmesh->insert( npc_entt, reg().get<Cmp::Position>( npc_entt ) ); }
  if ( not event.m_source_uuid.empty() )
  {
    SPDLOG_INFO( "Created SpawnedBy cmp: {}", event.m_source_uuid.str() );
    reg().emplace_or_replace<Cmp::Npc::SpawnedBy>( npc_entt, event.m_source_uuid );
  }
  if ( event.m_on_created ) { event.m_on_created( npc_entt ); }
}

void NpcSystem::on_crow_delivery_event( const Events::CrowDeliveryEvent &event )
{
  // one courier per item type: the player may activate again while the crow is still on its way
  for ( auto [crow_entt, crow_cmp] : reg().view<Cmp::Npc::Crow>().each() )
  {
    if ( crow_cmp.carried == entt::null or not reg().valid( crow_cmp.carried ) ) continue;
    const auto *carried_cmp = reg().try_get<Cmp::WorldItem>( crow_cmp.carried );
    if ( carried_cmp and carried_cmp->item_type == event.m_item ) return;
  }

  // the least worn matching item in the world. Items carried by NPCs or the player have no Cmp::Position
  entt::entity item_entt = entt::null;
  float best_level = -1.0f;
  for ( auto [cand_entt, cand_cmp, cand_pos_cmp] : reg().view<Cmp::WorldItem, Cmp::Position>( entt::exclude<Cmp::PlantMultiBlock> ).each() )
  {
    if ( cand_cmp.item_type != event.m_item ) continue;
    const auto *wear_cmp = reg().try_get<Cmp::Inventory::WearLevel>( cand_entt );
    const float level = wear_cmp ? wear_cmp->m_level : 100.0f;
    if ( level > best_level )
    {
      best_level = level;
      item_entt = cand_entt;
    }
  }
  if ( item_entt == entt::null )
  {
    SPDLOG_INFO( "No {} in the world for the crow to deliver", event.m_item.str() );
    return;
  }

  // the player can see it, so they can fetch it themselves
  const sf::FloatRect view_bounds = Utils::calculate_view_bounds( Sys::RenderSystem::get_world_view() );
  const sf::Vector2f item_centre = reg().get<Cmp::Position>( item_entt ).getCenter();
  if ( Utils::is_visible_in_view( view_bounds, reg().get<Cmp::Position>( item_entt ) ) )
  {
    SPDLOG_INFO( "{} is in view, no crow delivery", event.m_item.str() );
    return;
  }

  auto npc_navmesh = m_npc_navmesh.lock();
  auto ghost_navmesh = m_ghost_navmesh.lock();
  auto loot_sm = m_loot_sm.lock();
  auto reserved_sm = m_reserved_sm.lock();
  const Utils::SpawnGrids grids{ npc_navmesh.get(), ghost_navmesh.get(), loot_sm.get(), reserved_sm.get() };
  const Cmp::Position player_pos = Utils::Player::get_position( reg() );
  auto cell = Utils::find_random_cell_near( player_pos, kCrowDropRadius, [&]( const sf::Vector2f &cell_pos )
  { return Utils::is_free_spawn_area( reg(), sf::FloatRect( cell_pos, Constants::kGridSizePxF ), grids ); } );
  if ( not cell )
  {
    SPDLOG_WARN( "No free cell for the crow to drop {} near [{},{}]", event.m_item.str(), player_pos.position.x, player_pos.position.y );
    return;
  }

  // moved, not copied: the item leaves its old spot now and can't be picked up while it is carried
  lift_world_item( item_entt );

  // take off from where the item lay (always off screen), so the further away it is the longer the delivery takes
  const sf::Vector2f drop_centre = *cell + Constants::kGridSizePxF / 2.0f;
  const sf::Vector2f crow_size = npc_sprite_size( "npc.crow" );
  const sf::Vector2f start_centre = item_centre;
  const sf::Vector2f dir = ( drop_centre - start_centre ).normalized();

  // NpcSystem owns the factory. The crow flies over everything, so it never goes on a navmesh
  auto crow_entt = Factory::Npc::create_npc( reg(), Cmp::Position( start_centre - crow_size / 2.0f, crow_size ), "npc.crow" );
  if ( crow_entt == entt::null )
  {
    // better back where it was than lost
    place_world_item( item_entt, *cell );
    return;
  }
  apply_npc_spawn_action( crow_entt );
  auto &crow_cmp = reg().get<Cmp::Npc::Crow>( crow_entt );
  crow_cmp.carried = item_entt;
  crow_cmp.drop_position = *cell;
  crow_cmp.flight_direction = dir;
  reg().emplace_or_replace<Cmp::Direction>( crow_entt, dir );
  auto &anim_cmp = reg().get<Cmp::AnimData>( crow_entt );
  anim_cmp.m_sprite_type = "sprite.crow.fly";
  // the sprite faces east, so mirror it to fly west
  anim_cmp.m_flip_x = dir.x < 0.0f;
  anim_cmp.m_enabled = true;
  SPDLOG_INFO( "Crow {} delivering {} ({}%) to [{},{}]", static_cast<int>( crow_entt ), event.m_item.str(), best_level, cell->x, cell->y );
}

void NpcSystem::drop_crow_item( Cmp::Npc::Crow &crow_cmp )
{
  const entt::entity item_entt = std::exchange( crow_cmp.carried, entt::null );
  if ( not reg().valid( item_entt ) or not reg().all_of<Cmp::WorldItem>( item_entt ) ) return;

  // something may have landed on the drop cell during the flight
  auto npc_navmesh = m_npc_navmesh.lock();
  auto ghost_navmesh = m_ghost_navmesh.lock();
  auto loot_sm = m_loot_sm.lock();
  auto reserved_sm = m_reserved_sm.lock();
  const Utils::SpawnGrids grids{ npc_navmesh.get(), ghost_navmesh.get(), loot_sm.get(), reserved_sm.get() };
  const auto is_free = [&]( const sf::Vector2f &cell_pos )
  { return Utils::is_free_spawn_area( reg(), sf::FloatRect( cell_pos, Constants::kGridSizePxF ), grids ); };
  sf::Vector2f drop_pos = crow_cmp.drop_position;
  if ( not is_free( drop_pos ) )
  {
    // better on the original cell than lost
    drop_pos = Utils::find_random_cell_near( Cmp::Position( drop_pos, Constants::kGridSizePxF ), kCrowDropRadius, is_free ).value_or( drop_pos );
  }
  place_world_item( item_entt, drop_pos );
  m_sound_bank.get_effect( "drop_inventory" ).play();
}

void NpcSystem::lift_world_item( entt::entity item_entt )
{
  if ( auto reserved_sm = m_reserved_sm.lock() )
  {
    if ( const auto *pos_cmp = reg().try_get<Cmp::Position>( item_entt ) ) reserved_sm->remove( item_entt, *pos_cmp );
  }
  // keeps every other component (wear level, uuid, dowsing target...) for when it is put back
  reg().remove<Cmp::Position, Cmp::AnimData, Cmp::ZOrderValue>( item_entt );
}

void NpcSystem::place_world_item( entt::entity item_entt, sf::Vector2f cell )
{
  const Cmp::Position item_pos( cell, Constants::kGridSizePxF );
  reg().emplace_or_replace<Cmp::Position>( item_entt, item_pos );
  reg().emplace_or_replace<Cmp::AnimData>(
      item_entt, Cmp::AnimData::Config{ .sprite_type = reg().get<Cmp::WorldItem>( item_entt ).sprite_type, .enabled = false } );
  reg().emplace_or_replace<Cmp::ZOrderValue>( item_entt, cell.y - 1.f );
  if ( auto reserved_sm = m_reserved_sm.lock() ) reserved_sm->insert( item_entt, item_pos );
}

sf::Vector2f NpcSystem::npc_sprite_size( const Sys::NpcKey &npc_type )
{
  const auto &npc_cmp = Sys::NpcStore::instance().get( npc_type );
  return Sys::SpriteStore::instance().get( npc_cmp.sprite_type_list.front() ).sprite_size();
}

std::optional<sf::Vector2f> NpcSystem::find_nearby_spawn_cell( const Cmp::Position &source, const Sys::NpcKey &npc_type )
{
  const sf::Vector2f npc_size = npc_sprite_size( npc_type );
  auto npc_navmesh = m_npc_navmesh.lock();
  if ( not npc_navmesh ) return std::nullopt;
  auto ghost_navmesh = m_ghost_navmesh.lock();
  auto loot_sm = m_loot_sm.lock();
  auto reserved_sm = m_reserved_sm.lock();
  const Utils::SpawnGrids grids{ npc_navmesh.get(), ghost_navmesh.get(), loot_sm.get(), reserved_sm.get() };
  // the NPC's whole footprint must be free, not just its top-left cell
  return Utils::find_random_cell_near( source, kNpcSpawnSearchRadius, [&]( const sf::Vector2f &cell )
  { return Utils::is_free_spawn_area( reg(), sf::FloatRect( cell, npc_size ), grids ); } );
}

void NpcSystem::on_update_npc_position_event( const Events::UpdateNpcPositionEvent &event )
{
  const entt::entity npc_entity = event.m_npc_entity;
  if ( not reg().valid( npc_entity ) ) return;
  auto *pos_cmp = reg().try_get<Cmp::Position>( npc_entity );
  if ( not pos_cmp ) return;

  // The bucket the NPC is in is the cell it last set off from, not wherever it has lerped to since. A step that
  // hasn't started yet (zero lerp factor) has no m_start: the NPC is still in the cell it is standing on.
  Cmp::Position old_position = *pos_cmp;
  if ( auto *lerp_pos_cmp = reg().try_get<Cmp::LerpPosition>( npc_entity ) )
  {
    if ( lerp_pos_cmp->m_lerp_factor > 0.0f ) { old_position.position = lerp_pos_cmp->m_start; }
    // abandon the step, otherwise update_movement_for lerps the NPC straight back onto it
    reg().remove<Cmp::LerpPosition>( npc_entity );
  }

  pos_cmp->position = event.m_position;
  if ( auto navmesh = navmesh_for( npc_entity ) ) { navmesh->update( npc_entity, old_position, *pos_cmp ); }

  // update_movement_for only refreshes the zorder while the NPC is stepping
  if ( reg().all_of<Cmp::ZOrderValue>( npc_entity ) )
  {
    const float zorder = Sys::SpriteStore::instance().get( Utils::Npc::get_sprite_type( reg(), npc_entity ) ).zorder( 0, pos_cmp->position.y );
    reg().patch<Cmp::ZOrderValue>( npc_entity, [&]( auto &zorder_cmp ) { zorder_cmp.set( zorder ); } );
  }
}

void NpcSystem::on_destroy_npc_event( const Events::DestroyNpcEvent &event )
{
  const entt::entity npc_entity = event.m_npc_entity;
  if ( not reg().valid( npc_entity ) ) return;
  auto *pos_cmp = reg().try_get<Cmp::Position>( npc_entity );
  if ( not pos_cmp ) return;

  // only the axe or an arrow kills the Burryman, and he drops everything he stole
  if ( reg().all_of<Cmp::Npc::Burryman>( npc_entity ) )
  {
    using Cause = Events::DestroyNpcEvent::Cause;
    if ( event.m_cause != Cause::AXE and event.m_cause != Cause::ARROW ) return;
    drop_burryman_items( npc_entity, *pos_cmp );
  }

  // The bucket the NPC is in is the cell it last set off from, not wherever it has lerped to since. A step that
  // hasn't started yet (zero lerp factor) has no m_start: the NPC is still in the cell it is standing on.
  Cmp::Position bucket_position = *pos_cmp;
  if ( auto *lerp_pos_cmp = reg().try_get<Cmp::LerpPosition>( npc_entity ) )
  {
    if ( lerp_pos_cmp->m_lerp_factor > 0.0f ) { bucket_position.position = lerp_pos_cmp->m_start; }
  }

  // navmesh_for reads the NPC's type marker, so this has to happen before the entity is destroyed below
  if ( auto navmesh = navmesh_for( npc_entity ) ) { navmesh->remove( npc_entity, bucket_position ); }

  // the animation plays where the NPC is seen to be, which is its mid-step position rather than its bucket
  if ( event.m_death_anim ) { create_death_anim( *pos_cmp, *event.m_death_anim ); }
  if ( event.m_death_sfx ) { m_sound_bank.get_effect( *event.m_death_sfx ).play(); }

  if ( const auto *npc_cmp = reg().try_get<Cmp::Npc::NPC>( npc_entity ) )
  {
    apply_npc_action<Cmp::DestroyAction>( npc_cmp->npc_type, npc_cmp->at<Cmp::DestroyAction>().action );
  }
  Factory::Npc::destroy_npc( reg(), npc_entity );
}

entt::entity NpcSystem::create_death_anim( const Cmp::Position &npc_pos_cmp, const Sys::SpriteKey &death_anim )
{
  auto npc_death_entity = reg().create();
  // anchor the animation on the middle cell of the NPC's body (a no-op for single-block NPCs)
  const sf::Vector2f death_pos = npc_pos_cmp.position + ( npc_pos_cmp.size - Constants::kGridSizePxF ) * 0.5f;
  reg().emplace<Cmp::Position>( npc_death_entity, death_pos, Constants::kGridSizePxF );
  reg().emplace_or_replace<Cmp::DeathPosition>( npc_death_entity, death_pos, Constants::kGridSizePxF );

  // clang-format off
  reg().emplace_or_replace<Cmp::AnimData>( npc_death_entity, Cmp::AnimData::Config{ 
        .sprite_type = death_anim, 
        .enabled = true,
        .anim_type =  Cmp::AnimType::ONESHOTRESET
  });
  // clang-format on

  reg().emplace_or_replace<Cmp::ZOrderValue>( npc_death_entity, death_pos.y + 64 );
  return npc_death_entity;
}

void NpcSystem::update( sf::Time dt )
{

  // remove any finished death animations
  for ( auto [death_entt, death_cmp, anim_cmp] : reg().view<Cmp::DeathPosition, Cmp::AnimData>().each() )
  {
    if ( not anim_cmp.m_enabled ) { Factory::Npc::remove_npc_death_anim( reg(), death_entt ); }
  }

  static constexpr float kContainerInterval = 0.20f;
  m_container_timer += dt;
  if ( m_container_timer.asSeconds() >= kContainerInterval )
  {
    check_npc_container_collision();
    m_container_timer = sf::Time::Zero;
  }

  update_pathfinding( dt );

  // resting render offset, refreshed every frame so tuning applies immediately
  const sf::Vector2f npc_render_offset{ 0.f, Sys::PersistSystem::get<Cmp::Persist::NpcRenderOffset>( reg() ).get_value() };
  for ( auto npc_entt : reg().view<Cmp::Npc::NPC>() )
  {
    reg().emplace_or_replace<Cmp::AbsoluteRenderOffset>( npc_entt, npc_render_offset );
  }

  static constexpr float kAnimInterval = 0.05f;
  m_anim_timer += dt;
  if ( m_anim_timer.asSeconds() >= kAnimInterval )
  {
    update_animation();
    m_anim_timer = sf::Time::Zero;
  }
  update_sfx( dt );
  update_crows( dt );
  update_movement( dt );
  update_burryman_pickup( dt );

  // before the collision checks, so the ElderMother can't hurt the player on the frame she is banished
  check_elderflute_banish();
  check_plant_kill_npc( dt );
  check_npc_proximity( dt );

  if ( Utils::scene_setting<Cmp::SceneSettings::CollisionDetection>( reg() ).enabled )
  {
    check_once_collision();
    check_timed_collision( dt );
    check_burning_particle_collision();
  }

  // kill any expired mimics
  std::vector<entt::entity> kill_list;
  for ( auto [npc_entt, npc_cmp, npc_mimic_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Npc::Mimic>().each() )
  {
    npc_mimic_cmp.timer += dt;
    if ( npc_mimic_cmp.timer >= npc_mimic_cmp.timeout ) kill_list.push_back( npc_entt );
    reg().emplace_or_replace<Cmp::AbsoluteTint>( npc_entt, Sys::PersistSystem::get<Cmp::Persist::NpcMimicTint>( reg() ) );

    // flash as a despawn warning. TeleportSystem owns alpha while a teleport is pending - don't clobber its fade
    if ( reg().all_of<Cmp::PendingTeleport>( npc_entt ) ) continue;
    const bool is_flashing = npc_mimic_cmp.timeout - npc_mimic_cmp.timer <= npc_mimic_cmp.flash_duration;
    const bool flash_visible = ( npc_mimic_cmp.timer.asMilliseconds() / 100 ) % 2 == 0;
    reg().emplace_or_replace<Cmp::AbsoluteAlpha>( npc_entt, ( not is_flashing or flash_visible ) ? 255 : 0 );
  }
  for ( auto &mimic_npc_entt : kill_list )
  {
    // snapped so the player isn't teleported onto the mimic's mid-step position
    const sf::Vector2f mimic_pos = Utils::snap_to_grid( reg().get<Cmp::Position>( mimic_npc_entt ).position );
    const auto &mimic_cmp = reg().get<Cmp::Npc::Mimic>( mimic_npc_entt );
    const sf::Time teleport_delay = mimic_cmp.teleport_delay;
    on_destroy_npc_event( Events::DestroyNpcEvent( mimic_npc_entt ) );

    // the mimic has led the NPCs to its position, so being teleported there is a trap that luck [0,100] avoids
    const float teleport_chance = 1.f - ( static_cast<float>( Utils::Player::get_stats( reg() ).luck() ) / 100.f );
    if ( Cmp::RandomFloat( 0.f, 1.f ).gen() < teleport_chance )
    {
      get_systems_event_queue().trigger( Events::TeleportEntityEvent( Utils::Player::get_entity( reg() ), mimic_pos,
                                                                      Sys::SpriteKey( "sprite.death.anim.banish" ), std::nullopt, teleport_delay ) );
    }
  }

  update_azazel( dt );
}

void NpcSystem::update_azazel( sf::Time dt )
{
  const auto fireball_tag = std::string( Cmp::Particle::LavaFireballParticleSprite::kAzazelTag );
  // remove any spent fireballs
  Factory::Particle::delete_expired_particle_sprites( reg(), fireball_tag );

  const sf::Time fire_cooldown = sf::seconds( Sys::PersistSystem::get<Cmp::Persist::AzazelFireballCooldown>( reg() ).get_value() );
  const float fire_speed = Sys::PersistSystem::get<Cmp::Persist::AzazelFireballSpeed>( reg() ).get_value();
  const float fire_arc = Sys::PersistSystem::get<Cmp::Persist::AzazelFireballArc>( reg() ).get_value();
  auto fire_palette = Cmp::Particle::LavaFireballParticleSprite::kPurplePalette;
  fire_palette.body = Sys::PersistSystem::get<Cmp::Persist::AzazelFireballColour>( reg() );
  const auto &world_view = RenderSystem::get_world_view();
  const sf::Vector2f player_centre = Utils::Player::get_position( reg() ).getCenter();

  // destroying an NPC modifies the registry, so collect the despawns rather than destroying them mid-iteration
  std::vector<entt::entity> kill_list;
  for ( auto [npc_entt, azazel_cmp, pos_cmp, spawned_by_cmp] : reg().view<Cmp::Npc::Azazel, Cmp::Position, Cmp::Npc::SpawnedBy>().each() )
  {
    // Azazel is banished once every offering dropped by its summoning offering (world item or carried) has been offered
    const auto offerings = reg().view<Cmp::Npc::SummonOffering>();
    const bool has_offerings = std::ranges::any_of( offerings, [&]( entt::entity offering_entt )
    { return offerings.get<Cmp::Npc::SummonOffering>( offering_entt ).m_summoner == spawned_by_cmp.m_source; } );
    if ( not has_offerings )
    {
      kill_list.push_back( npc_entt );
      continue;
    }

    azazel_cmp.fire_cooldown += dt;
    if ( azazel_cmp.fire_cooldown < fire_cooldown ) continue;
    if ( not Utils::is_visible_in_view( world_view, pos_cmp ) ) continue;
    azazel_cmp.fire_cooldown = sf::Time::Zero;

    // a shallow arc onto the player, low enough that obstacles in between still block it (see ParticleSystem::check_collsion)
    auto fireball_uuid = Cmp::UUID::generate();
    Factory::Particle::add_aimed_fireball_ps( reg(), fireball_tag, 3.f, fire_speed, fire_arc, 4.f, fireball_uuid, pos_cmp.getCenter(), player_centre,
                                              pos_cmp.position.y + 64.f, fire_palette );
  }
  for ( auto azazel_entt : kill_list )
  {
    SPDLOG_INFO( "Azazel entity {} banished: all offerings offered", static_cast<int>( azazel_entt ) );
    on_destroy_npc_event( Events::DestroyNpcEvent( azazel_entt ) );
  }
}

void NpcSystem::check_npc_container_collision()
{

  auto player_pos = Utils::Player::get_position( reg() );
  auto npccontainer_collision_view = reg().view<Cmp::Npc::Container, Cmp::Position>();
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [npccontainer_entt, npccontainer_cmp, npccontainer_pos_cmp] : npccontainer_collision_view.each() )
  {
    if ( !Utils::is_visible_in_view( view_bounds, npccontainer_pos_cmp ) ) continue;

    auto &npc_activate_scale = Sys::PersistSystem::get<Cmp::Persist::NpcActivateScale>( reg() );
    // we just create a temporary RectBounds here instead of a component because we only need it
    // for this one comparison and it already contains the needed scaling logic
    auto npc_activate_bounds = Cmp::RectBounds::scaled( npccontainer_pos_cmp.position, Constants::kGridSizePxF, npc_activate_scale.get_value() );

    if ( player_pos.findIntersection( npc_activate_bounds.getBounds() ) )
    {
      auto npc_entt = Factory::Npc::create_npc( reg(), npccontainer_pos_cmp, "npc.skeleton" );
      apply_npc_spawn_action( npc_entt );
      if ( auto navmesh = navmesh_for( npc_entt ); navmesh and npc_entt != entt::null )
      {
        navmesh->insert( npc_entt, reg().get<Cmp::Position>( npc_entt ) );
      }
      Factory::Npc::destroy_npc_container( reg(), npccontainer_entt, m_reserved_sm.lock() );
    }
  }
}

void NpcSystem::update_animation()
{
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );

  for ( auto [npc_entt, npc_cmp, npc_dir_cmp, anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Direction, Cmp::AnimData>().each() )
  {
    // crows pace their grave or fly off, even while the dowsing rod blinds everything else, see update_crows
    if ( reg().any_of<Cmp::Npc::Crow>( npc_entt ) ) continue;

    // dowsing rod "blinds" all NPC pathfinding, except wisps: let them walk out their current step, then stand still
    if ( inventory_type.contains( "dowsingrod" ) and not reg().any_of<Cmp::Npc::Wisp>( npc_entt ) )
    {
      if ( reg().all_of<Cmp::LerpPosition>( npc_entt ) ) continue;
      npc_dir_cmp = Cmp::Direction( { 0.0f, 0.0f } );
      anim_cmp.m_enabled = false;
      continue;
    }

    // Watchmen face wherever their searchlight is currently pointing — sweeping, patrolling a
    // cardinal direction, or locked onto the player — even while standing still, rather than
    // freezing on whatever direction they last walked.
    if ( auto *searchlight_cmp = reg().try_get<Cmp::Npc::WatchmanSearchlight>( npc_entt ) )
    {
      anim_cmp.m_enabled = npc_dir_cmp != sf::Vector2f( 0.0f, 0.0f );
      const sf::Vector2f facing = searchlight_cmp->cone_direction;
      if ( std::abs( facing.x ) > std::abs( facing.y ) )
      {
        anim_cmp.m_sprite_type = facing.x > 0 ? "sprite.nightwatchman.walk.east" : "sprite.nightwatchman.walk.west";
      }
      else { anim_cmp.m_sprite_type = facing.y < 0 ? "sprite.nightwatchman.walk.north" : "sprite.nightwatchman.walk.south"; }
      continue;
    }

    if ( npc_dir_cmp == sf::Vector2f( 0.0f, 0.0f ) )
    {
      anim_cmp.m_enabled = false;
      continue;
    }

    anim_cmp.m_enabled = true;
    if ( anim_cmp.m_sprite_type.contains( "sprite.skeleton" ) )
    {

      if ( npc_dir_cmp.x > 0 ) { anim_cmp.m_sprite_type = "sprite.skeleton.walk.east"; }
      else if ( npc_dir_cmp.x < 0 ) { anim_cmp.m_sprite_type = "sprite.skeleton.walk.west"; }
      else if ( npc_dir_cmp.y < 0 ) { anim_cmp.m_sprite_type = "sprite.skeleton.walk.north"; }
      else if ( npc_dir_cmp.y > 0 ) { anim_cmp.m_sprite_type = "sprite.skeleton.walk.south"; }
    }
    else if ( anim_cmp.m_sprite_type.contains( "sprite.ghost" ) )
    {
      if ( npc_dir_cmp.x > 0 ) { anim_cmp.m_sprite_type = "sprite.ghost.walk.east"; }
      else if ( npc_dir_cmp.x < 0 ) { anim_cmp.m_sprite_type = "sprite.ghost.walk.west"; }
      else if ( npc_dir_cmp.y < 0 ) { anim_cmp.m_sprite_type = "sprite.ghost.walk.north"; }
      else if ( npc_dir_cmp.y > 0 ) { anim_cmp.m_sprite_type = "sprite.ghost.walk.south"; }
    }
    else if ( anim_cmp.m_sprite_type.contains( "sprite.wisp" ) )
    {
      if ( npc_dir_cmp.x > 0 ) { anim_cmp.m_sprite_type = "sprite.wisp.east"; }
      else if ( npc_dir_cmp.x < 0 ) { anim_cmp.m_sprite_type = "sprite.wisp.west"; }
    }
    else if ( anim_cmp.m_sprite_type.contains( "sprite.player.walk" ) )
    {
      if ( npc_dir_cmp.x > 0 ) { anim_cmp.m_sprite_type = "sprite.player.walk.east"; }
      else if ( npc_dir_cmp.x < 0 ) { anim_cmp.m_sprite_type = "sprite.player.walk.west"; }
      else if ( npc_dir_cmp.y < 0 ) { anim_cmp.m_sprite_type = "sprite.player.walk.north"; }
      else if ( npc_dir_cmp.y > 0 ) { anim_cmp.m_sprite_type = "sprite.player.walk.south"; }
    }
    else if ( anim_cmp.m_sprite_type.contains( "sprite.eldermother" ) )
    {
      if ( npc_dir_cmp.x > 0 ) { anim_cmp.m_flip_x = false; }
      else if ( npc_dir_cmp.x < 0 ) { anim_cmp.m_flip_x = true; }
    }
  }
}

void NpcSystem::update_sfx( sf::Time dt )
{
  bool any_skeleton_moving = false;
  bool any_spider_moving = false;

  // snake hiss
  static const sf::Time snake_hiss_cooldown_timeout = sf::seconds( 5.f );
  for ( auto [adder_entt, adder_cmp, adder_pos_cmp] : reg().view<Cmp::Npc::Adder, Cmp::Position>().each() )
  {
    adder_cmp.sfx_cooldown += dt;
    if ( adder_cmp.sfx_cooldown < snake_hiss_cooldown_timeout ) continue;
    if ( Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), adder_pos_cmp ) )
    {
      auto sfx = std::string( "snake_hiss" ) + std::to_string( Cmp::RandomInt( 1, 7 ).gen() );
      m_sound_bank.get_effect( sfx ).play();
    }
    adder_cmp.sfx_cooldown = sf::Time::Zero;
  }

  for ( auto [npc_entt, npc_cmp, npc_dir_cmp, anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Direction, Cmp::AnimData>().each() )
  {
    if ( anim_cmp.m_sprite_type.contains( "sprite.skeleton" ) )
    {
      if ( npc_dir_cmp != sf::Vector2f{ 0.0f, 0.0f } )
      {
        any_skeleton_moving = true;
        break;
      }
    }
    else if ( anim_cmp.m_sprite_type.contains( "sprite.spider" ) )
    {
      if ( npc_dir_cmp != sf::Vector2f{ 0.0f, 0.0f } )
      {
        any_spider_moving = true;
        break;
      }
    }
  }

  auto &skeleton_sfx = m_sound_bank.get_effect( "skeleton_moving" );
  if ( any_skeleton_moving )
  {
    if ( skeleton_sfx.getStatus() != sf::Sound::Status::Playing ) { skeleton_sfx.play(); }
  }
  else { skeleton_sfx.stop(); }

  auto &spider_sfx = m_sound_bank.get_effect( "spider_moving" );
  if ( any_spider_moving )
  {
    if ( spider_sfx.getStatus() != sf::Sound::Status::Playing ) { spider_sfx.play(); }
  }
  else { spider_sfx.stop(); }

  std::vector<std::string> crow_calls{ "crow1", "crow2", "crow3", "crow4", "crow5" };
  auto crow_call_picker = Cmp::RandomInt( 0, crow_calls.size() - 1 );
  for ( auto [crow_entt, crow_cmp, pos_cmp] : reg().view<Cmp::Npc::Crow, Cmp::Position>().each() )
  {
    sf::Time kCrowCallTimeout = crow_cmp.flee ? sf::seconds( 0.5f ) : sf::seconds( 3.f );
    crow_cmp.sfx_cooldown += dt;
    if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), pos_cmp ) ) continue;
    if ( crow_cmp.sfx_cooldown >= kCrowCallTimeout )
    {
      auto &crow_sfx = m_sound_bank.get_effect( crow_calls.at( crow_call_picker.gen() ) );
      if ( crow_sfx.getStatus() != sf::Sound::Status::Playing ) { crow_sfx.play(); }
      crow_cmp.sfx_cooldown = sf::Time::Zero;
    }
  }
}

void NpcSystem::update_crows( sf::Time dt )
{
  const float flight_speed = Constants::kGridSizePxF.x * Sys::PersistSystem::get<Cmp::Persist::NpcCrowFlightSpeed>( reg() ).get_value();
  const float perch_speed = Sys::PersistSystem::get<Cmp::Persist::NpcCrowPerchSpeed>( reg() ).get_value();
  const float perch_range = Sys::PersistSystem::get<Cmp::Persist::NpcCrowPerchRange>( reg() ).get_value();
  const float max_perch_wait = Sys::PersistSystem::get<Cmp::Persist::NpcCrowMaxPerchWait>( reg() ).get_value();
  const sf::FloatRect view_bounds = Utils::calculate_view_bounds( Sys::RenderSystem::get_world_view() );
  const Cmp::Position player_pos = Utils::Player::get_position( reg() );

  std::vector<entt::entity> kill_list;
  for ( auto [crow_entt, crow_cmp, pos_cmp, anim_cmp] : reg().view<Cmp::Npc::Crow, Cmp::Position, Cmp::AnimData>().each() )
  {
    // courier: fly in to the drop cell, drop the item, then fly back out the way it came
    if ( crow_cmp.carried != entt::null and crow_cmp.flight_direction )
    {
      const sf::Vector2f drop_centre = crow_cmp.drop_position + Constants::kGridSizePxF / 2.0f;
      const float step = flight_speed * dt.asSeconds();
      const float remaining = ( drop_centre - pos_cmp.getCenter() ).dot( *crow_cmp.flight_direction );
      if ( remaining > step ) { pos_cmp.position += *crow_cmp.flight_direction * step; }
      else
      {
        pos_cmp.position = drop_centre - pos_cmp.size / 2.0f;
        drop_crow_item( crow_cmp );
        crow_cmp.flee = true;
        crow_cmp.flight_direction = -*crow_cmp.flight_direction;
        reg().emplace_or_replace<Cmp::Direction>( crow_entt, *crow_cmp.flight_direction );
        anim_cmp.m_flip_x = crow_cmp.flight_direction->x < 0.0f;
      }
      const float zorder = Sys::SpriteStore::instance().get( anim_cmp.m_sprite_type ).zorder( 0, pos_cmp.position.y );
      reg().patch<Cmp::ZOrderValue>( crow_entt, [&]( auto &zorder_cmp ) { zorder_cmp.set( zorder ); } );
      // still flying in from off screen, so no off-view kill check
      continue;
    }

    if ( not crow_cmp.flee )
    {
      // the crow only animates while standing still. AnimSystem only steps an NPC's frames while its Cmp::Direction
      // is non-zero, so the standing crow is given a nominal direction and the moving crow a zero one
      const auto set_perch_direction = [&]()
      {
        const bool moving = crow_cmp.perch_target_x.has_value();
        reg().emplace_or_replace<Cmp::Direction>( crow_entt, moving ? sf::Vector2f( 0.0f, 0.0f ) : sf::Vector2f( 1.0f, 0.0f ) );
        anim_cmp.m_enabled = not moving;
        // moving: hold the first frame
        if ( moving )
        {
          anim_cmp.m_current_frame = 0;
          anim_cmp.m_elapsed_time = sf::Time::Zero;
        }
      };
      set_perch_direction();
      if ( crow_cmp.perch_wait > sf::Time::Zero )
      {
        crow_cmp.perch_wait -= dt;
        continue;
      }

      // done moving or waiting: either wait a while, or move to a random spot within perch_range of the grave centre
      if ( not crow_cmp.perch_target_x )
      {
        if ( Cmp::RandomInt( 0, 1 ).gen() == 0 ) { crow_cmp.perch_wait = sf::seconds( Cmp::RandomFloat( 0.0f, max_perch_wait ).gen() ); }
        else
        {
          crow_cmp.perch_target_x = crow_cmp.perch_center_x + Cmp::RandomFloat( -perch_range, perch_range ).gen();
          // the sprite faces east, so mirror it to walk west
          anim_cmp.m_flip_x = *crow_cmp.perch_target_x < pos_cmp.getCenter().x;
        }
        set_perch_direction();
        continue;
      }

      const Cmp::Position old_position = pos_cmp;
      const float remaining = *crow_cmp.perch_target_x - pos_cmp.getCenter().x;
      const float step = perch_speed * dt.asSeconds();
      if ( std::abs( remaining ) <= step )
      {
        pos_cmp.position.x = *crow_cmp.perch_target_x - pos_cmp.size.x / 2.0f;
        crow_cmp.perch_target_x.reset();
        set_perch_direction();
      }
      else { pos_cmp.position.x += std::copysign( step, remaining ); }
      if ( auto navmesh = navmesh_for( crow_entt ) ) { navmesh->update( crow_entt, old_position, pos_cmp ); }
      continue;
    }
    if ( not crow_cmp.flight_direction )
    {
      // take off: the crow flies over everything, so it leaves the navmesh and abandons any step it was walking.
      // The bucket it is in is the cell it last set off from, not wherever it has lerped to since.
      Cmp::Position bucket_position = pos_cmp;
      if ( auto *lerp_pos_cmp = reg().try_get<Cmp::LerpPosition>( crow_entt ) )
      {
        if ( lerp_pos_cmp->m_lerp_factor > 0.0f ) { bucket_position.position = lerp_pos_cmp->m_start; }
        reg().remove<Cmp::LerpPosition>( crow_entt );
      }
      if ( auto navmesh = navmesh_for( crow_entt ) ) { navmesh->remove( crow_entt, bucket_position ); }

      // flee away from the player, straight up if the player is standing right on it
      const sf::Vector2f away = pos_cmp.getCenter() - player_pos.getCenter();
      crow_cmp.flight_direction = away.lengthSquared() > 0.0f ? away.normalized() : sf::Vector2f( 0.0f, -1.0f );
      reg().emplace_or_replace<Cmp::Direction>( crow_entt, *crow_cmp.flight_direction );
      anim_cmp.m_sprite_type = "sprite.crow.fly";
      anim_cmp.m_flip_x = crow_cmp.flight_direction->x < 0.0f;
      anim_cmp.m_enabled = true;
    }

    pos_cmp.position += *crow_cmp.flight_direction * flight_speed * dt.asSeconds();
    const float zorder = Sys::SpriteStore::instance().get( anim_cmp.m_sprite_type ).zorder( 0, pos_cmp.position.y );
    reg().patch<Cmp::ZOrderValue>( crow_entt, [&]( auto &zorder_cmp ) { zorder_cmp.set( zorder ); } );

    if ( not Utils::is_visible_in_view( view_bounds, pos_cmp ) ) kill_list.push_back( crow_entt );
  }
  // already off the navmesh, so the event's navmesh removal is a no-op
  for ( auto crow_entt : kill_list )
  {
    on_destroy_npc_event( Events::DestroyNpcEvent( crow_entt ) );
  }
}

void NpcSystem::update_pathfinding( sf::Time dt )
{
  // dowsing rod "blinds" all NPC pathfinding. Steps already in progress are left for update_movement to finish.
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type.contains( "dowsingrod" ) and Utils::Player::get_inventory_wear_level( reg() ) > 0 ) return;

  static constexpr float kPathfindingInterval = 0.10f;
  m_pathfinding_timer += dt;
  if ( m_pathfinding_timer.asSeconds() >= kPathfindingInterval )
  {
    // NPCs — target is the player, unless the player repels them (e.g. blood oil): then they flee out of view
    const Cmp::Position player_pos = Utils::Player::get_position( reg() );
    const bool player_is_illuminated = Utils::Player::is_illuminated( reg() );
    const bool repel = reg().all_of<Cmp::Player::RepelNpc>( Utils::Player::get_entity( reg() ) );
    // a mimic diverts all NPC pathfinding away from the player and onto itself
    std::optional<Cmp::Position> mimic_pos;
    for ( auto [mimic_entt, mimic_cmp, mimic_pos_cmp] : reg().view<Cmp::Npc::Mimic, Cmp::Position>().each() )
    {
      mimic_pos = mimic_pos_cmp;
      break;
    }
    const sf::FloatRect view_bounds = Utils::calculate_view_bounds( Sys::RenderSystem::get_world_view() );
    const auto lights = Utils::Collision::collect_light_sources( reg() );
    for ( auto [npc_entt, npc_cmp] : reg().view<Cmp::Npc::NPC>().each() )
    {
      if ( reg().any_of<Cmp::Npc::Wisp>( npc_entt ) ) continue;
      // crows never chase: they pace their grave, then fly straight off screen, see update_crows
      if ( reg().any_of<Cmp::Npc::Crow>( npc_entt ) ) continue;

      // Watchmen stand sentry, sweeping their searchlight, until they actually catch the player
      // in the beam — only then do they give chase.
      auto *searchlight_cmp = reg().try_get<Cmp::Npc::WatchmanSearchlight>( npc_entt );
      if ( searchlight_cmp and not searchlight_cmp->locked_on_player )
      {
        reg().emplace_or_replace<Cmp::Direction>( npc_entt, Cmp::Direction( { 0.0f, 0.0f } ) );
        continue;
      }

      // Adders lie in wait, only striking out at whatever they would chase (the mimic, else the player) once it strays within range
      if ( reg().all_of<Cmp::Npc::Adder>( npc_entt ) )
      {
        static constexpr float kAdderPathfindRange = Constants::kGridSizePxF.x * 3.0f;
        const Cmp::Position &adder_target = ( mimic_pos and not repel ) ? *mimic_pos : player_pos;
        const auto &npc_pos_cmp = reg().get<Cmp::Position>( npc_entt );
        if ( Utils::Maths::getChebyshevDistance( npc_pos_cmp.position, adder_target.position ) > kAdderPathfindRange )
        {
          reg().emplace_or_replace<Cmp::Direction>( npc_entt, Cmp::Direction( { 0.0f, 0.0f } ) );
          continue;
        }
      }

      auto navmesh = navmesh_for( npc_entt );
      if ( not navmesh ) continue;

      // the Burryman never chases: he steals what he can reach, then flees the player
      if ( reg().all_of<Cmp::Npc::Burryman>( npc_entt ) )
      {
        update_burryman_pathfinding( *navmesh, npc_entt, player_pos, view_bounds );
        continue;
      }

      // the mimic flees out of view, drawing the NPCs that chase it away from the player.
      // Lights are ignored: they only hold back threats, and the mimic may spawn inside the player's torch light.
      if ( reg().all_of<Cmp::Npc::Mimic>( npc_entt ) )
      {
        const auto &npc_pos_cmp = reg().get<Cmp::Position>( npc_entt );
        auto result = Utils::Npc::pathfind_toward( reg(), *navmesh, repel_target( player_pos, npc_pos_cmp, view_bounds ), npc_entt, false, false,
                                                   false, nullptr, true );
        if ( result == Utils::Npc::PathfindResult::NoPath )
        {
          reg().emplace_or_replace<Cmp::Direction>( npc_entt, Cmp::Direction( { 0.0f, 0.0f } ) );
        }
        continue;
      }

      if ( repel )
      {
        // the flee target is not lit by the player's light
        const auto &npc_pos_cmp = reg().get<Cmp::Position>( npc_entt );
        update_pathfinding_for( *navmesh, repel_target( player_pos, npc_pos_cmp, view_bounds ), npc_entt, false, lights );
      }
      else if ( mimic_pos ) { update_pathfinding_for( *navmesh, *mimic_pos, npc_entt, false, lights ); }
      else { update_pathfinding_for( *navmesh, player_pos, npc_entt, player_is_illuminated, lights ); }
    }

    m_pathfinding_timer = sf::Time::Zero;
  }
}
void NpcSystem::update_pathfinding_for( PathFinding::SpatialHashGrid &navmesh, const Cmp::Position &target_pos, entt::entity npc_entity,
                                        bool target_illuminated, const Utils::Collision::LightSources &lights )
{
  // No spawn-boundary stop for player-chasing NPCs: light sources now provide the player's protection.
  // An unreachable player (e.g. flying over an obstacle) is approached as closely as possible rather than given up on.
  auto result = Utils::Npc::pathfind_toward( reg(), navmesh, target_pos, npc_entity, false, target_illuminated, false, &lights, true );
  if ( result == Utils::Npc::PathfindResult::NoPath ) { reg().emplace_or_replace<Cmp::Direction>( npc_entity, Cmp::Direction( { 0.0, 0.0 } ) ); }
}

Cmp::Position NpcSystem::repel_target( const Cmp::Position &player_pos, const Cmp::Position &npc_pos, const sf::FloatRect &view_bounds ) const
{
  // aim one cell past the view edge so the NPC ends up fully out of view
  const sf::FloatRect bounds( view_bounds.position - Constants::kGridSizePxF, view_bounds.size + Constants::kGridSizePxF * 2.f );
  const sf::Vector2f origin = player_pos.getCenter();
  sf::Vector2f dir = npc_pos.getCenter() - origin;
  // NPC on top of the player: flee toward the farthest view corner
  if ( dir == sf::Vector2f( 0.f, 0.f ) )
  {
    const sf::Vector2f view_center = bounds.getCenter();
    const sf::Vector2f corner( origin.x < view_center.x ? bounds.position.x + bounds.size.x : bounds.position.x,
                               origin.y < view_center.y ? bounds.position.y + bounds.size.y : bounds.position.y );
    dir = corner - origin;
  }

  // nearest exit through the x and y slabs of the bounds
  float t = std::numeric_limits<float>::max();
  if ( dir.x > 0.f ) { t = std::min( t, ( bounds.position.x + bounds.size.x - origin.x ) / dir.x ); }
  else if ( dir.x < 0.f ) { t = std::min( t, ( bounds.position.x - origin.x ) / dir.x ); }
  if ( dir.y > 0.f ) { t = std::min( t, ( bounds.position.y + bounds.size.y - origin.y ) / dir.y ); }
  else if ( dir.y < 0.f ) { t = std::min( t, ( bounds.position.y - origin.y ) / dir.y ); }
  if ( t == std::numeric_limits<float>::max() ) { t = 0.f; }

  const sf::Vector2f target_center = origin + dir * t;
  return Cmp::Position( target_center - player_pos.size * 0.5f, player_pos.size );
}

void NpcSystem::update_burryman_pathfinding( PathFinding::SpatialHashGrid &navmesh, entt::entity npc_entity, const Cmp::Position &player_pos,
                                             const sf::FloatRect &view_bounds )
{
  const auto &npc_pos_cmp = reg().get<Cmp::Position>( npc_entity );

  // the nearest on-screen world items, planted plants and rotten food excepted
  std::vector<std::pair<float, Cmp::Position>> candidates;
  for ( auto [item_entt, item_cmp, item_pos_cmp] : reg().view<Cmp::WorldItem, Cmp::Position>( entt::exclude<Cmp::PlantMultiBlock> ).each() )
  {
    if ( item_cmp.item_type.str() == kBurrymanIgnoredItem ) continue;
    if ( not Utils::is_visible_in_view( view_bounds, item_pos_cmp ) ) continue;
    const sf::Vector2f delta = item_pos_cmp.getCenter() - npc_pos_cmp.getCenter();
    candidates.emplace_back( delta.lengthSquared(), item_pos_cmp );
  }
  const std::size_t candidate_count = std::min( candidates.size(), kBurrymanMaxCandidates );
  std::partial_sort( candidates.begin(), candidates.begin() + static_cast<std::ptrdiff_t>( candidate_count ), candidates.end(),
                     []( const auto &lhs, const auto &rhs ) { return lhs.first < rhs.first; } );

  // Item cells may be missing from the navmesh (Cmp::Npc::NoPathFinding), so aim for a walkable cell next to the
  // item instead: update_burryman_pickup() collects it from there. No path to any of them means no clear path.
  const std::array<sf::Vector2f, 4> neighbours{ sf::Vector2f( 0.f, -Constants::kGridSizePxF.y ), sf::Vector2f( Constants::kGridSizePxF.x, 0.f ),
                                                sf::Vector2f( 0.f, Constants::kGridSizePxF.y ), sf::Vector2f( -Constants::kGridSizePxF.x, 0.f ) };
  for ( std::size_t i = 0; i < candidate_count; ++i )
  {
    const sf::Vector2f item_cell = Utils::snap_to_grid( candidates[i].second.position );
    for ( const auto &offset : neighbours )
    {
      const Cmp::Position target( item_cell + offset, Constants::kGridSizePxF );
      if ( navmesh.at( target ).empty() ) continue;
      auto result = Utils::Npc::pathfind_toward( reg(), navmesh, target, npc_entity, false, false, false, nullptr, false );
      if ( result != Utils::Npc::PathfindResult::NoPath ) return;
    }
  }

  // nothing left within reach: flee out of view, ignoring lights like the mimic
  auto result = Utils::Npc::pathfind_toward( reg(), navmesh, repel_target( player_pos, npc_pos_cmp, view_bounds ), npc_entity, false, false, false,
                                             nullptr, true );
  if ( result == Utils::Npc::PathfindResult::NoPath ) { reg().emplace_or_replace<Cmp::Direction>( npc_entity, Cmp::Direction( { 0.0f, 0.0f } ) ); }
}

void NpcSystem::update_burryman_pickup( sf::Time dt )
{
  for ( auto [npc_entt, burryman_cmp, npc_pos_cmp] : reg().view<Cmp::Npc::Burryman, Cmp::Position>().each() )
  {
    burryman_cmp.sfx_cooldown += dt;

    // reaches into the cells around him, diagonals included
    const auto reach = Cmp::RectBounds::scaled( npc_pos_cmp, 1.5f );

    // collect first: removing Cmp::Position below modifies the view's storage
    std::vector<entt::entity> picked_up;
    for ( auto [item_entt, item_cmp, item_pos_cmp] : reg().view<Cmp::WorldItem, Cmp::Position>( entt::exclude<Cmp::PlantMultiBlock> ).each() )
    {
      if ( item_cmp.item_type.str() == kBurrymanIgnoredItem ) continue;
      if ( reach.findIntersection( item_pos_cmp ) ) picked_up.push_back( item_entt );
    }
    if ( picked_up.empty() ) continue;

    for ( auto item_entt : picked_up )
    {
      lift_world_item( item_entt );
      burryman_cmp.carried.push_back( item_entt );
      SPDLOG_INFO( "Burryman {} picked up {}", static_cast<int>( npc_entt ), reg().get<Cmp::WorldItem>( item_entt ).item_type );
    }

    static const sf::Time kPickupSfxCooldown = sf::milliseconds( 500 );
    if ( burryman_cmp.sfx_cooldown >= kPickupSfxCooldown )
    {
      m_sound_bank.get_effect( "equip_inventory" ).play();
      burryman_cmp.sfx_cooldown = sf::Time::Zero;
    }
  }
}

void NpcSystem::drop_burryman_items( entt::entity burryman_entity, const Cmp::Position &burryman_pos )
{
  auto &burryman_cmp = reg().get<Cmp::Npc::Burryman>( burryman_entity );

  auto npc_navmesh = m_npc_navmesh.lock();
  auto ghost_navmesh = m_ghost_navmesh.lock();
  auto loot_sm = m_loot_sm.lock();
  auto reserved_sm = m_reserved_sm.lock();
  const Utils::SpawnGrids grids{ npc_navmesh.get(), ghost_navmesh.get(), loot_sm.get(), reserved_sm.get() };

  // scatter the items: each one gets its own free cell around him
  std::vector<sf::Vector2f> used_cells;
  bool dropped_any = false;
  for ( auto item_entt : burryman_cmp.carried )
  {
    // the item may have worn away while he carried it
    if ( not reg().valid( item_entt ) or not reg().all_of<Cmp::WorldItem>( item_entt ) ) continue;

    auto cell = Utils::find_random_cell_near( burryman_pos, kBurrymanDropRadius, [&]( const sf::Vector2f &cell_pos )
    {
      if ( std::ranges::find( used_cells, cell_pos ) != used_cells.end() ) return false;
      return Utils::is_free_spawn_area( reg(), sf::FloatRect( cell_pos, Constants::kGridSizePxF ), grids );
    } );
    // better on top of him than lost
    const sf::Vector2f drop_pos = cell ? *cell : Utils::snap_to_grid( burryman_pos.position );
    used_cells.push_back( drop_pos );

    place_world_item( item_entt, drop_pos );
    dropped_any = true;
  }
  burryman_cmp.carried.clear();
  if ( dropped_any ) m_sound_bank.get_effect( "drop_inventory" ).play();
}

void NpcSystem::update_movement( sf::Time dt )
{
  for ( auto [npc_entt, npc_cmp, npc_pos_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position>().each() )
  {
    auto navmesh = navmesh_for( npc_entt );
    if ( not navmesh ) continue;
    update_movement_for( *navmesh, npc_entt, dt );
  }
}

PathFinding::SpatialHashGridSharedPtr NpcSystem::navmesh_for( entt::entity npc_entity )
{
  if ( reg().any_of<Cmp::Npc::Wisp>( npc_entity ) or reg().any_of<Cmp::Npc::ElderMother>( npc_entity ) ) { return m_open_navmesh.lock(); }
  if ( reg().any_of<Cmp::Npc::Ghost>( npc_entity ) )
  {
    // ghosts pass through plants; fall back to the regular navmesh in scenes without one
    if ( auto ghost_navmesh = m_ghost_navmesh.lock() ) { return ghost_navmesh; }
  }
  return m_npc_navmesh.lock();
}

void NpcSystem::update_movement_for( PathFinding::SpatialHashGrid &navmesh, entt::entity npc_entity, sf::Time dt )
{

  // if there is an obstacle at this entity move onto the next entity
  auto *obst_cmp = reg().try_get<Cmp::Obstacle>( npc_entity );
  if ( obst_cmp ) return;

  // If this is the first update, store the start position
  auto *lerp_pos_cmp = reg().try_get<Cmp::LerpPosition>( npc_entity );
  auto *pos_cmp = reg().try_get<Cmp::Position>( npc_entity );
  if ( not pos_cmp or not lerp_pos_cmp ) return;
  if ( lerp_pos_cmp->m_lerp_factor == 0.0f )
  {
    // Allow NPCs to escape wormholes if they're mid-lerp, otherwise hold them still while a teleport is pending.
    if ( reg().all_of<Cmp::PendingTeleport>( npc_entity ) ) return;

    lerp_pos_cmp->m_start = pos_cmp->position;
  }

  // re-evaluated every frame so the NPC speeds back up mid-step as burrs expire
  const float burr_multiplier = Utils::get_burr_speed_multiplier( reg(), npc_entity, *pos_cmp );
  lerp_pos_cmp->m_lerp_factor += lerp_pos_cmp->m_lerp_speed * burr_multiplier * dt.asSeconds();

  // lerp has completed
  if ( lerp_pos_cmp->m_lerp_factor >= 1.0f )
  {
    // the cell the NPC set off from, which is the bucket it is in: its mid-lerp position has already crossed
    // into the target cell when moving left or up
    const Cmp::Position old_position( lerp_pos_cmp->m_start, pos_cmp->size );
    pos_cmp->position = lerp_pos_cmp->m_target;
    reg().remove<Cmp::LerpPosition>( npc_entity );
    navmesh.update( npc_entity, old_position, *pos_cmp );
  }
  else
  {
    // Lerp from start to target directly
    // Simple manual lerp - 33 lines of assembly vs 134 for std::lerp vs 54 for std::fma - https://godbolt.org/z/YdeKco5d6
    const float t = lerp_pos_cmp->m_lerp_factor;
    const float one_minus_t = 1.0f - t;
    pos_cmp->position.x = one_minus_t * lerp_pos_cmp->m_start.x + t * lerp_pos_cmp->m_target.x;
    pos_cmp->position.y = one_minus_t * lerp_pos_cmp->m_start.y + t * lerp_pos_cmp->m_target.y;
  }

  // the sprite's json zorder resolved against the y-axis position: wisps use it to float above everything,
  // multiblock NPCs use a RelativeZOrder to move their sorting line down from the top-left anchor to their feet
  const float zorder = Sys::SpriteStore::instance().get( Utils::Npc::get_sprite_type( reg(), npc_entity ) ).zorder( 0, pos_cmp->position.y );
  reg().patch<Cmp::ZOrderValue>( npc_entity, [&]( auto &zorder_cmp ) { zorder_cmp.set( zorder ); } );
}

void NpcSystem::check_elderflute_banish()
{
  auto [inventory_entt, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type != "item.elderflute" or Utils::Player::get_inventory_wear_level( reg() ) <= 0 ) return;

  // the distance efficacy where the elder flute can banish the elder mother
  auto half_view = Cmp::RectBounds::scaled( Utils::calculate_view_bounds( RenderSystem::get_world_view() ), 0.25f );

  for ( auto [npc_entt, em_cmp, npc_pos_cmp] : reg().view<Cmp::Npc::ElderMother, Cmp::Position>().each() )
  {
    if ( not Utils::is_visible_in_view( half_view.getBounds(), npc_pos_cmp ) ) continue;
    get_systems_event_queue().trigger( Events::DestroyNpcEvent( npc_entt, Sys::SpriteKey( "sprite.death.anim.banish" ), "witches_jar_offering" ) );
  }
}

bool NpcSystem::is_flying_over( const sf::FloatRect &npc_bounds )
{
  if ( not Utils::Player::is_flying( reg() ) ) return false;
  return npc_bounds.size.x <= Constants::kGridSizePxF.x and npc_bounds.size.y <= Constants::kGridSizePxF.y;
}

void NpcSystem::check_once_collision()
{
  auto player_collision_view = reg().view<Cmp::Player::Character>();
  auto npc_collision_view = reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::Direction>( entt::exclude<Cmp::Npc::Friendly> );

  const auto &player_pos = Utils::Player::get_position( reg() );
  auto &player_mort = Utils::Player::get_mortality( reg() );
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [player_entity, player_cmp] : player_collision_view.each() )
  {
    if ( player_mort.state != Cmp::Player::Mortality::State::ALIVE ) return;
    for ( auto [npc_entity, npc_cmp, npc_pos_cmp, npc_dir_cmp] : npc_collision_view.each() )
    {
      if ( not Utils::is_visible_in_view( view_bounds, npc_pos_cmp ) ) continue;

      // relaxed bounds to allow player to sneak past during lerp transition. Inset by a fixed margin rather
      // than scaled, so a multiblock NPC keeps a hitbox covering its body instead of a dot at its centre.
      // Only the cells a multiblock NPC stands on collide: the rows above them are off the ground.
      const auto npc_collision_bounds = Utils::Npc::collision_bounds( reg(), npc_entity );
      if ( not npc_collision_bounds ) continue;
      const sf::FloatRect &npc_bounds = *npc_collision_bounds;
      const sf::Vector2f touch_inset = Constants::kGridSizePxF * 0.45f;
      const sf::FloatRect npc_touch_bounds( npc_bounds.position + touch_inset, npc_bounds.size - touch_inset * 2.f );
      const bool touching_player = player_pos.findIntersection( npc_touch_bounds ).has_value();

      // a Watchman that's touched the player obviously knows exactly where they are — snap its
      // searchlight (and, since sprite facing already follows cone_direction, its sprite too) to
      // face them directly. This must happen on every physical touch, independent of the
      // player's damage-cooldown/NPC collision-action-interval gates below — those only throttle
      // *damage*, and gating detection on them meant a touch during someone else's cooldown
      // would silently fail to turn the Watchman.
      if ( touching_player )
      {
        if ( auto *searchlight_cmp = reg().try_get<Cmp::Npc::WatchmanSearchlight>( npc_entity ) )
        {
          if ( auto to_player = Utils::Maths::normalized( player_pos.getCenter() - npc_pos_cmp.getCenter() ); to_player.has_value() )
          {
            searchlight_cmp->locked_on_player = true;
            searchlight_cmp->cone_direction = *to_player;
          }
        }
      }

      // a flying player is out of reach of 1x1 NPCs
      if ( is_flying_over( npc_bounds ) ) continue;

      if ( not player_cmp.skip_damage_cooldown_once && player_cmp.m_damage_cooldown_timer < player_cmp.damage_cooldown_timeout() ) continue;

      const auto &action = npc_cmp.at<Cmp::CollisionAction>().action;
      if ( action.interval() > 0.f ) continue;

      if ( not touching_player ) continue;

      get_systems_event_queue().trigger( Events::NpcCollisionEvent( npc_cmp.npc_type, player_pos ) );
      reg().emplace_or_replace<Cmp::Player::TookDamage>( Utils::Player::get_entity( reg() ) );
      player_cmp.skip_damage_cooldown_once = false;

      m_sound_bank.get_effect( "damage_player" ).play();

      if ( check_player_death( player_mort ) ) return;

      player_cmp.m_damage_cooldown_timer = sf::Time::Zero;

      find_pushback_position( npc_dir_cmp, npc_bounds );
    }
  }
}

void NpcSystem::check_timed_collision( sf::Time dt )
{
  auto npc_collision_view = reg().view<Cmp::Npc::NPC, Cmp::Position>( entt::exclude<Cmp::Npc::Friendly> );

  const auto &player_pos = Utils::Player::get_position( reg() );
  auto &player_mort = Utils::Player::get_mortality( reg() );
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  if ( player_mort.state != Cmp::Player::Mortality::State::ALIVE ) return;
  for ( auto [npc_entity, npc_cmp, npc_pos_cmp] : npc_collision_view.each() )
  {
    if ( not Utils::is_visible_in_view( view_bounds, npc_pos_cmp ) ) continue;
    auto &npc_collision_action = npc_cmp.at<Cmp::CollisionAction>();

    auto &[npc_action, npc_action_timer] = npc_collision_action;
    if ( npc_action.interval() == 0.f ) continue;
    npc_action_timer += dt;
    if ( npc_action_timer.asSeconds() < npc_action.interval() ) continue;

    // only the cells a multiblock NPC stands on collide: the rows above them are off the ground
    const auto npc_collision_bounds = Utils::Npc::collision_bounds( reg(), npc_entity );
    if ( not npc_collision_bounds or not player_pos.findIntersection( *npc_collision_bounds ) ) continue;
    // a flying player is out of reach of 1x1 NPCs
    if ( is_flying_over( *npc_collision_bounds ) ) continue;

    get_systems_event_queue().trigger( Events::NpcCollisionEvent( npc_cmp.npc_type, player_pos ) );
    reg().emplace_or_replace<Cmp::Player::TookDamage>( Utils::Player::get_entity( reg() ) );
    if ( m_sound_bank.get_effect( "damage_player" ).getStatus() != sf::Sound::Status::Playing ) { m_sound_bank.get_effect( "damage_player" ).play(); }

    npc_action_timer = sf::Time::Zero;

    if ( check_player_death( player_mort ) ) return;
  }
}

void NpcSystem::check_plant_kill_npc( sf::Time dt )
{
  const int player_luck = Utils::Player::get_stats( reg() ).luck();
  // destroyed after the loop: the event handler modifies this storage
  std::vector<entt::entity> worn_out_plants;
  for ( auto [plant_entt, plant_mb_cmp, plant_worlditem_cmp, plant_uuid] : reg().view<Cmp::PlantMultiBlock, Cmp::WorldItem, Cmp::UUID>().each() )
  {
    const auto &proximity_action = plant_worlditem_cmp.get_action<Cmp::ProximityAction>();
    const auto &kill_npc = proximity_action.side_effects().kill_npc;
    if ( not kill_npc ) continue;

    auto &timer = reg().get_or_emplace<Cmp::Plant::KillNpcTimer>( plant_entt );
    timer += dt;
    if ( timer.asSeconds() < proximity_action.interval() ) continue;
    timer = Cmp::Plant::KillNpcTimer{};

    // collect first: the event handler modifies this storage
    const auto kill_bounds = Cmp::RectBounds::expanded( plant_mb_cmp, kill_npc->trigger_range );
    // don't kill more NPCs than the plant's remaining wear allows
    std::size_t kills_left = std::numeric_limits<std::size_t>::max();
    if ( const auto *wearlevel_cmp = reg().try_get<Cmp::Inventory::WearLevel>( plant_entt ); wearlevel_cmp and kill_npc->wear_per_kill > 0.f )
    {
      kills_left = static_cast<std::size_t>( std::ceil( wearlevel_cmp->m_level / kill_npc->wear_per_kill ) );
    }
    std::vector<entt::entity> npcs;
    for ( auto [npc_entt, npc_cmp, npc_pos_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position>().each() )
    {
      if ( npcs.size() >= kills_left ) break;
      if ( not kill_npc->targets( npc_cmp.npc_type ) ) continue;
      // a plant doesn't kill its own spawns, e.g. red campion adders
      if ( const auto *spawned_by_cmp = reg().try_get<Cmp::Npc::SpawnedBy>( npc_entt ); spawned_by_cmp and spawned_by_cmp->m_source == plant_uuid )
        continue;
      if ( not kill_bounds.findIntersection( npc_pos_cmp ).has_value() ) continue;
      if ( not kill_npc->roll( player_luck ) ) continue;
      npcs.push_back( npc_entt );
    }
    if ( npcs.empty() ) continue;

    for ( auto npc_entt : npcs )
    {
      // each kill plays the plant's proximity_action "final" sfx (see items.json)
      const auto &kill_sfx = proximity_action.sfx().final;
      get_systems_event_queue().trigger( Events::DestroyNpcEvent( npc_entt, kill_npc->death_anim,
                                                                  kill_sfx.empty() ? std::nullopt : std::optional<std::string>( kill_sfx ) ) );
      if ( kill_npc->wear_per_kill > 0.f ) get_systems_event_queue().trigger( Events::UpdateDmgEvent( plant_entt, kill_npc->wear_per_kill ) );
    }
    get_systems_event_queue().trigger( Events::GrimoireItemUpdateEvent( plant_worlditem_cmp.item_type, Cmp::ProximityAction::kKey, proximity_action ) );
    // a plant worn out by its kills withers away, like a fully foraged one
    if ( Utils::is_plant_depleted( reg(), plant_entt ) ) worn_out_plants.push_back( plant_entt );
  }
  for ( auto plant_entt : worn_out_plants )
  {
    get_systems_event_queue().trigger( Events::DestroyPlantEvent( plant_entt ) );
  }
}

void NpcSystem::check_burning_particle_collision()
{
  std::vector<Cmp::Particle::IParticleSprite *> burning_sprites;
  for ( auto [ps_entt, ps_owner] : reg().view<Cmp::Particle::SpriteOwner>().each() )
  {
    if ( not ps_owner.sprite->is_active() ) continue;
    const auto tag = ps_owner.sprite->get_tag();
    if ( not tag.contains( Cmp::Particle::LavaFireballParticleSprite::kTagSuffix ) and
         not tag.contains( Cmp::Particle::EmberParticleSprite::kTagSuffix ) )
      continue;
    burning_sprites.push_back( ps_owner.sprite.get() );
  }
  if ( burning_sprites.empty() ) return;

  // destroying an NPC modifies the registry, so take a snapshot of the hit NPCs rather than destroying them mid-iteration
  std::vector<entt::entity> burnt_npcs;
  for ( auto [npc_entt, npc_cmp, npc_pos_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position>( entt::exclude<Cmp::Npc::Ghost> ).each() )
  {
    bool hit = false;
    for ( auto *burning_sprite : burning_sprites )
    {
      // the collision check also kills the particle that hit the NPC
      if ( burning_sprite->check_particle_collision( npc_pos_cmp ) ) hit = true;
    }
    if ( hit ) burnt_npcs.push_back( npc_entt );
  }

  for ( entt::entity npc_entt : burnt_npcs )
  {
    SPDLOG_INFO( "NPC entity {} killed by burning particle", static_cast<int>( npc_entt ) );
    get_systems_event_queue().trigger( Events::DestroyNpcEvent( npc_entt, Sys::SpriteKey( "sprite.death.anim.flames" ) ) );
  }
}

bool NpcSystem::check_player_death( Cmp::Player::Mortality &player_mort )
{
  if ( Utils::Player::get_stats( reg() ).health() > 0 ) return false;

  player_mort.state = Cmp::Player::Mortality::State::HAUNTED;
  get_systems_event_queue().enqueue( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::HAUNTED, Utils::Player::get_position( reg() ) ) );
  return true;
}

void NpcSystem::find_pushback_position( const Cmp::Direction &npc_direction, const sf::FloatRect &npc_bounds )
{
  const auto &player_pos = Utils::Player::get_position( reg() );

  // returns the name of the component blocking `rect`, or an empty string if it's clear
  auto blocking_component_name = [&]( const Cmp::RectBounds &rect ) -> std::string
  {
    // a flying player is knocked back over 1x1 obstacles - only walls, boundaries and large multiblocks stop them
    if ( Utils::Player::is_flying( reg() ) )
    {
      for ( auto [nopath_entt, nopath_pos] : reg().view<Cmp::Player::NoPath, Cmp::Position>().each() )
      {
        if ( rect.findIntersection( nopath_pos ) and FlyingSystem::blocks_flight( reg(), nopath_entt ) ) return "FlightBlocker";
      }
      return {};
    }
    if ( Utils::Collision::any_intersects<Cmp::Obstacle>( reg(), rect ) ) return "Obstacle";
    if ( Utils::Collision::any_intersects<Cmp::PlantSegment>( reg(), rect ) ) return "PlantSegment";
    if ( Utils::Collision::any_intersects<Cmp::Wall>( reg(), rect ) ) return "Wall";
    if ( Utils::Collision::any_intersects<Cmp::Altar::Segment>( reg(), rect ) ) return "Segment";
    if ( Utils::Collision::any_intersects<Cmp::Grave::Segment>( reg(), rect ) ) return "Segment";
    if ( Utils::Collision::any_intersects<Cmp::Grave::ExitSegment>( reg(), rect ) ) return "ExitSegment";
    if ( Utils::Collision::any_intersects<Cmp::Crypt::BuildingSegment>( reg(), rect ) ) return "BuildingSegment";
    if ( Utils::Collision::any_intersects<Cmp::Ruin::BuildingSegment>( reg(), rect ) ) return "BuildingSegment";
    if ( Utils::Collision::any_intersects<Cmp::Crypt::ObjectiveSegment>( reg(), rect ) ) return "ObjectiveSegment";
    return {};
  };

  // Try the NPC's own approach direction first; if that cell is blocked, fall back through
  // the remaining cardinal directions instead of giving up on knockback entirely.
  const sf::Vector2f primary_direction = npc_direction;

  std::vector<sf::Vector2f> candidate_directions{ primary_direction };
  for ( auto c : Utils::Cardinal() )
  {
    if ( c.vector() != primary_direction ) candidate_directions.push_back( c.vector() );
  }

  // A single-block NPC knocks the player back one cell. A multiblock NPC can still cover that cell, so keep
  // stepping until the player is clear of its body.
  const sf::Vector2f npc_cells = npc_bounds.size.componentWiseDiv( Constants::kGridSizePxF );
  const float npc_max_cells = std::max( npc_cells.x, npc_cells.y );
  const int max_steps = npc_max_cells > 1.f ? static_cast<int>( std::ceil( npc_max_cells ) ) + 1 : 1;

  for ( const auto &direction : candidate_directions )
  {
    for ( int step = 1; step <= max_steps; ++step )
    {
      auto new_position = Utils::snap_to_grid( player_pos.position +
                                               direction.componentWiseMul( Constants::kGridSizePxF ) * static_cast<float>( step ) );
      auto new_pos_rect = Cmp::RectBounds::scaled( new_position, Constants::kGridSizePxF, 1.f );
      if ( step < max_steps and new_pos_rect.findIntersection( npc_bounds ) ) continue;

      auto blocker = blocking_component_name( new_pos_rect );
      if ( not blocker.empty() )
      {
        SPDLOG_INFO( "Knockback direction {},{} blocked by {} at {},{}", direction.x, direction.y, blocker, new_position.x, new_position.y );
        break;
      }

      SPDLOG_INFO( "Knockback succeeded: player moved to {},{}", new_position.x, new_position.y );
      get_systems_event_queue().trigger( Events::UpdatePlayerPositionEvent( new_position ) );
      return;
    }
  }

  SPDLOG_INFO( "Knockback blocked in all directions" );
}

} // namespace Game::Sys