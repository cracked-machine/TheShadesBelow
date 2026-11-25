#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteOffset.hpp>
#include <Components/AbsoluteRotation.hpp>
#include <Components/DroppedBy.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Health.hpp>
#include <Components/Inventory/StingDamage.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Loot.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/SpawnedBy.hpp>
#include <Components/Particle/EmberParticleSprite.hpp>
#include <Components/Particle/SpriteOwner.hpp>
#include <Components/Persistent/GlovesWearPerForage.hpp>
#include <Components/Persistent/PlantBurnTime.hpp>
#include <Components/Persistent/PlantWearPerForage.hpp>
#include <Components/Plant/BurningTimer.hpp>
#include <Components/Plant/Grass.hpp>
#include <Components/Player/DiggingTimer.hpp>
#include <Components/Position.hpp>
#include <Components/SelectedPosition.hpp>
#include <Components/Stats/ActionReward.hpp>
#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ForageAction.hpp>
#include <Components/Stats/SideEffects.hpp>
#include <Components/UUID.hpp>
#include <Components/WorldTile.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/CreatePlantEvent.hpp>
#include <Events/DestroyLootEvent.hpp>
#include <Events/DestroyNpcEvent.hpp>
#include <Events/DestroyPlantEvent.hpp>
#include <Events/PlantKilledEvent.hpp>
#include <Events/WeaponHitEvent.hpp>
#include <Events/GrimoireItemUpdateEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PlantFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/PlantSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Audio/Sound.hpp>
#include <SFML/System/Time.hpp>
#include <algorithm>
#include <cmath>
#include <entt/entity/registry.hpp>
#include <spdlog/spdlog.h>
#include <vector>

namespace Game::Sys
{

PlantSystem::PlantSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  // The entt::dispatcher is independent of the registry, so it is safe to bind event handlers in
  // the constructor
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&PlantSystem::on_player_action>( this );
  std::ignore = get_systems_event_queue().sink<Events::CreatePlantEvent>().connect<&PlantSystem::on_create_plant_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::DestroyPlantEvent>().connect<&PlantSystem::on_destroy_plant_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::PlantKilledEvent>().connect<&PlantSystem::on_plant_killed_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::WeaponHitEvent>().connect<&PlantSystem::on_weapon_hit_event>( this );
  SPDLOG_DEBUG( "PlantSystem initialized" );
}

void PlantSystem::update( sf::Time dt )
{
  // destroy particle sprite entities once all their particles have expired
  Factory::Particle::delete_expired_particle_sprites( reg(), "graveyard.plant.particle.leaves" );
  Factory::Particle::delete_expired_particle_sprites( reg(), "graveyard.plant.particle.twigs" );
  Factory::Particle::delete_expired_particle_sprites( reg(), std::string( Cmp::Particle::EmberParticleSprite::kYewTag ) );

  update_burning_plants( dt );
}

void PlantSystem::on_player_action( const Events::PlayerActionEvent &event )
{
  if ( event.action == Events::PlayerActionEvent::GameActions::DIG ) { player_plant_interact(); }
  else if ( event.action == Events::PlayerActionEvent::GameActions::BURN ) { try_burn_plant(); }
}

void PlantSystem::on_create_plant_event( const Events::CreatePlantEvent &event )
{
  auto plant_entt = gen_plant( event.m_plant_type, event.m_position );
  if ( plant_entt != entt::null and event.m_on_created ) event.m_on_created( plant_entt );
}

void PlantSystem::on_destroy_plant_event( const Events::DestroyPlantEvent &event )
{
  const auto plant_entt = event.m_plant_entity;
  if ( not reg().valid( plant_entt ) or not reg().all_of<Cmp::PlantMultiBlock, Cmp::UUID>( plant_entt ) ) return;
  const auto plant_uuid_cmp = reg().get<Cmp::UUID>( plant_entt );

  auto npc_navmesh = m_npc_navmesh.lock();
  auto player_navmesh = m_player_navmesh.lock();
  // grass never reserved its tile (see gen_plant), so any reservation on its cell belongs to something else
  auto reserved_sm = reg().all_of<Cmp::Plant::Grass>( plant_entt ) ? nullptr : m_reserved_sm.lock();

  for ( auto [seg_entt, plant_segment_cmp, seg_pos_cmp, seg_uuid_cmp] : reg().view<Cmp::PlantSegment, Cmp::Position, Cmp::UUID>().each() )
  {
    if ( seg_uuid_cmp != plant_uuid_cmp ) continue;

    // segments are indexed in the player navmesh as blockers - drop them before destroying,
    // otherwise is_valid_move queries stale entity handles
    if ( player_navmesh ) player_navmesh->remove( seg_entt, seg_pos_cmp );

    // Un-reserve the world tiles this plant claimed in create_multiblock_segments, otherwise
    // replanting here silently skips segment creation. Match tiles against the segment's
    // position so reservations held by neighbouring structures are untouched.
    if ( not reserved_sm ) continue;
    // only this segment and the world tile under it - anything else reserved on the cell belongs to someone else
    reserved_sm->remove( seg_entt, seg_pos_cmp );
    for ( auto tile_entt : reserved_sm->at( seg_pos_cmp ) )
    {
      if ( not reg().valid( tile_entt ) or not reg().all_of<Cmp::WorldTile>( tile_entt ) ) continue;
      reserved_sm->remove( tile_entt, seg_pos_cmp );
      // the tile is walkable again for NPCs, unless it blocks in its own right (e.g. an obstacle)
      if ( npc_navmesh and not reg().any_of<Cmp::Npc::NoPathFinding>( tile_entt ) ) { npc_navmesh->insert( tile_entt, seg_pos_cmp ); }
    }
  }

  // segments and the multiblock are now out of the navmeshes, so destroy them
  Factory::Plant::remove_plant_mb( reg(), plant_entt );
}

entt::entity PlantSystem::gen_plant( const Sys::ItemKey &plant_type, const Cmp::Position &pos_cmp )
{
  auto reserved_sm = m_reserved_sm.lock();
  if ( not reserved_sm )
  {
    SPDLOG_WARN( "gen_plant: no reserved spatial map, skipping {}", plant_type.str() );
    return entt::null;
  }
  const auto &pos = pos_cmp.position;

  const auto &item_cmp = Sys::ItemStore::instance().get( plant_type );
  const auto &plant_spritesheet = Sys::SpriteStore::instance().get( item_cmp.sprite_type );

  // Plants can span more than one grid row (e.g. 1x2), so every cell in the footprint - not
  // just the origin - must be a real, unreserved world tile. Otherwise a plant can overlap a
  // reserved tile it doesn't share an entity with, e.g. the player spawn.
  auto world_tile_sm = m_world_tile_sm.lock();
  // dropped items and hazard cells aren't structures, but a plant mustn't grow over them either
  auto item_sm = m_item_sm.lock();
  auto hazard_sm = m_hazard_sm.lock();
  const auto occupied_by = [&]( const PathFinding::SpatialHashGridSharedPtr &grid, const Cmp::Position &cell_pos )
  { return grid and std::ranges::any_of( grid->at( cell_pos ), [&]( entt::entity e ) { return reg().valid( e ); } ); };
  const auto is_world_tile = [&]( const Cmp::Position &cell_pos )
  {
    if ( not world_tile_sm ) return Utils::get_world_pos_entt( reg(), cell_pos ) != entt::null;
    return std::ranges::any_of( world_tile_sm->at( cell_pos ), [&]( entt::entity e ) { return reg().valid( e ); } );
  };
  auto plant_grid_size = plant_spritesheet.get_grid_size();
  bool footprint_clear = true;
  for ( int gy = 0; gy < plant_grid_size.y and footprint_clear; ++gy )
  {
    for ( int gx = 0; gx < plant_grid_size.x; ++gx )
    {
      sf::Vector2f pos_offset( static_cast<float>( gx ) * Constants::kGridSizePxF.x, static_cast<float>( gy ) * Constants::kGridSizePxF.y );
      Cmp::Position cell_pos( pos + pos_offset, Constants::kGridSizePxF );
      if ( not is_world_tile( cell_pos ) or not reserved_sm->at( cell_pos ).empty() or occupied_by( item_sm, cell_pos ) or
           occupied_by( hazard_sm, cell_pos ) )
      {
        footprint_clear = false;
        break;
      }
    }
  }

  if ( footprint_clear )
  {
    // grass covers most of the free ground, so it doesn't reserve its tile: hazards, drops and spawns may still use
    // the cell, and remove the grass when they do (see Factory::Plant::destroy_grass_in())
    const bool is_grass = plant_type.str() == Cmp::Plant::kGrassItem;
    // now create the plant at a new entt
    auto [mb_entt, _] = Factory::Multiblock::add_multiblock_with_segments<Cmp::PlantMultiBlock, Cmp::PlantSegment>(
        reg(), pos, plant_spritesheet, item_cmp.sprite_index(), /*zorder=*/0, is_grass ? nullptr : reserved_sm.get(), world_tile_sm.get() );
    if ( is_grass ) { reg().emplace_or_replace<Cmp::Plant::Grass>( mb_entt ); }

    // Add the worlditem now so we don't have to look it up later when digging up the plant
    const auto &plant_item = Sys::ItemStore::instance().get( plant_type );
    reg().emplace_or_replace<Cmp::WorldItem>( mb_entt, plant_item );
    // weapon hits, foraging, decay and kills all drain the plant's health
    reg().emplace_or_replace<Cmp::Health>( mb_entt, 100.f );
    if ( plant_item.stings() ) { reg().emplace_or_replace<Cmp::Inventory::StingDamage>( mb_entt ); }

    // Every covered cell is already reserved (except grass): create_multiblock_segments() inserts each segment into reserved_sm.
    return mb_entt;
  }
  return entt::null;
}

void PlantSystem::player_plant_interact()
{

  // abort if still in cooldown
  if ( Utils::Player::is_digging_on_cooldown( reg() ) ) { return; }

  // Cooldown has expired: Remove any existing SelectedPosition components from the registry
  Utils::reset_all_selected_positions( reg() );

  auto mouse_position_bounds = Utils::get_mouse_bounds_in_gameview( m_window, RenderSystem::get_world_view() );

  auto position_view = reg().view<Cmp::PlantMultiBlock, Cmp::UUID>( entt::exclude<Cmp::SelectedPosition> );
  for ( auto [plant_entt, plant_mb_cmp, plant_uuid_cmp] : position_view.each() )
  {
    if ( mouse_position_bounds.findIntersection( plant_mb_cmp ) )
    {
      // check player is near obstacle that was mouse-selected
      if ( not Utils::Player::is_player_near( reg(), plant_mb_cmp ) ) continue;

      // check player is facing the obstacle
      if ( not Utils::Player::get_projected_position( reg() ).findIntersection( plant_mb_cmp ) ) continue;

      // We are in proximity to an entity that is a candidate for a new SelectedPosition component.
      // Add a new SelectedPosition component to the entity
      reg().emplace_or_replace<Cmp::SelectedPosition>( plant_entt, plant_mb_cmp.position );
      reg().emplace_or_replace<Cmp::Player::DiggingTimer>( Utils::Player::get_entity( reg() ) );

      if ( not reg().all_of<Cmp::WorldItem>( plant_entt ) ) continue;

      auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
      // an empty hand, gloves, or the plant's own forage item (the forage joins the held stack) forage the plant instead of hitting it
      const auto forage_output = reg().get<Cmp::WorldItem>( plant_entt ).get_forage_output();
      const bool holding_forage_output = not forage_output.str().empty() and inventory_type == forage_output;
      if ( inventory_type.str().empty() or inventory_type == "item.gloves" or holding_forage_output )
      {
        forage_plant( plant_entt, plant_mb_cmp, inventory_type == "item.gloves" );
        return;
      }
      if ( inventory_type != "item.shovel" ) continue;

      if ( Utils::Player::get_inventory_wear_level( reg() ) <= 0 ) { return; }

      // Prevent destroying a plant on fire
      if ( reg().any_of<Cmp::Plant::BurningTimer>( plant_entt ) ) continue;

      auto plantleaves_particle_uuid = Cmp::UUID::generate();
      Factory::Particle::add_plantleaves_ps( reg(), "graveyard.plant.particle.leaves", 50, 2.f, 50.f, 14.f, plantleaves_particle_uuid,
                                             plant_mb_cmp.getCenter(), plant_mb_cmp.position.y );
      auto planttwigs_particle_uuid = Cmp::UUID::generate();
      Factory::Particle::add_planttwigs_ps( reg(), "graveyard.plant.particle.twigs", 10, 2.f, 50.f, 14.f, planttwigs_particle_uuid,
                                            plant_mb_cmp.getCenter(), plant_mb_cmp.position.y );

      on_weapon_hit_event( Events::WeaponHitEvent( plant_entt, "item.shovel", plant_mb_cmp.getCenter() - Utils::Player::get_position( reg() ).getCenter() ) );
      return;
    }
  }
}

void PlantSystem::forage_plant( entt::entity plant_entt, const Cmp::PlantMultiBlock &plant_mb_cmp, bool wearing_gloves )
{
  // Prevent foraging a plant on fire
  if ( reg().any_of<Cmp::Plant::BurningTimer>( plant_entt ) ) return;

  // copy the reward and sfx: creating items or removing the plant below can invalidate the component references.
  // Plants without a forage_action.reward.item (see items.json) can't be foraged.
  const auto &plant_item = reg().get<Cmp::WorldItem>( plant_entt );
  const auto forage_reward = plant_item.get_reward<Cmp::ForageAction>();
  if ( not forage_reward ) return;
  const auto forage_sfx = plant_item.get_action<Cmp::ForageAction>().sfx();
  const auto plant_type = plant_item.item_type;
  const auto plant_mb_copy = plant_mb_cmp;
  const Cmp::Position plant_bounds( plant_mb_copy.position, plant_mb_copy.size );
  const Cmp::Position plant_pos_cmp( plant_mb_copy.position, Constants::kGridSizePxF );

  auto plantleaves_particle_uuid = Cmp::UUID::generate();
  Factory::Particle::add_plantleaves_ps( reg(), "graveyard.plant.particle.leaves", 50, 2.f, 50.f, 14.f, plantleaves_particle_uuid,
                                         plant_mb_copy.getCenter(), plant_mb_copy.position.y );

  // the forage is dropped beside the plant: the inventory may be occupied. It is a reward for the player, so it may be
  // picked up straight into an empty slot (items.json "auto_pickup"). Gloves stop the drop stinging.
  if ( forage_reward->target == Cmp::ActionReward::Target::DEFAULT )
  {
    get_systems_event_queue().trigger( Events::CreateItemEvent( Cmp::Position( find_drop_pos( plant_mb_copy ), Constants::kGridSizePxF ),
                                                                Sys::ItemStore::instance().pick_item( forage_reward->item ), "drop_loot", 0.f,
                                                                false, 0, entt::null, {}, /*sting=*/not wearing_gloves, /*player_reward=*/true ) );
  }
  else { grant_item_reward( *forage_reward, plant_bounds, plant_entt ); }
  play_action_sfx( forage_sfx );

  // each forage drains the plant's health
  const auto plant_wear = Sys::PersistSystem::get<Cmp::Persist::PlantWearPerForage>( reg() ).get_value();
  get_systems_event_queue().trigger( Events::UpdateDmgEvent( plant_entt, plant_wear ) );

  if ( wearing_gloves )
  {
    const auto gloves_wear = Sys::PersistSystem::get<Cmp::Persist::GlovesWearPerForage>( reg() ).get_value();
    get_systems_event_queue().trigger( Events::UpdateDmgEvent( Utils::Player::get_inventory_entt( reg() ), gloves_wear ) );
  }

  // Gloves block the forage_action's sting (its health damage) but not its benefits or side effects
  const auto apply_forage_action = [&]() -> Cmp::BaseAction
  {
    if ( not wearing_gloves ) { return Utils::Player::apply_action_from_world_item<Cmp::ForageAction>( reg(), plant_entt ); }
    Cmp::BaseAction action = reg().get<Cmp::WorldItem>( plant_entt ).get_action<Cmp::ForageAction>();
    action.remove_damage();
    Utils::Player::get_stats( reg() ).apply( action );
    get_systems_event_queue().trigger( Events::GrimoireItemUpdateEvent( plant_type, Cmp::ForageAction::kKey, action ) );
    return action;
  };
  const auto forage_action = apply_forage_action();
  trigger_side_effects( forage_action, plant_pos_cmp, plant_entt );

  // a fully foraged plant withers away - no drops
  if ( reg().valid( plant_entt ) and Utils::is_plant_depleted( reg(), plant_entt ) )
  {
    get_systems_event_queue().trigger( Events::DestroyPlantEvent( plant_entt ) );
  }
}

void PlantSystem::on_weapon_hit_event( const Events::WeaponHitEvent &event )
{
  const auto plant_entt = event.m_target;
  if ( not reg().valid( plant_entt ) or not reg().all_of<Cmp::PlantMultiBlock, Cmp::WorldItem, Cmp::Health>( plant_entt ) ) return;

  // the hit takes the plant's damage_from_weapon from its health and wears the weapon by its damage_to_weapon
  const auto &plant_item = reg().get<Cmp::WorldItem>( plant_entt );
  const auto destroy_sfx = plant_item.get_action<Cmp::DestroyAction>().sfx();
  const auto damage = static_cast<float>( plant_item.damage_from_weapon().get( event.m_weapon ) );
  Utils::Player::wear_held_weapon( reg(), event.m_weapon, plant_item.damage_to_weapon() );
  if ( damage <= 0.f ) return;

  if ( Utils::Player::apply_damage( reg(), plant_entt, damage ) )
  {
    on_plant_killed_event( Events::PlantKilledEvent( plant_entt ) );
    return;
  }
  // intermediate hits play the destroy_action "oneshot" sfx, the final hit its "final" sfx
  play_action_oneshot_sfx( destroy_sfx );
}

void PlantSystem::on_plant_killed_event( const Events::PlantKilledEvent &event )
{
  const auto plant_entt = event.m_plant_entity;
  if ( not reg().valid( plant_entt ) or not reg().all_of<Cmp::PlantMultiBlock, Cmp::WorldItem, Cmp::UUID>( plant_entt ) ) return;

  // copy everything needed: creating items or removing the plant below can invalidate the component references
  const auto plant_mb_cmp = reg().get<Cmp::PlantMultiBlock>( plant_entt );
  const auto plant_uuid_cmp = reg().get<Cmp::UUID>( plant_entt );
  const auto destroy_reward = reg().get<Cmp::WorldItem>( plant_entt ).get_reward<Cmp::DestroyAction>();
  const Cmp::Position plant_bounds( plant_mb_cmp.position, plant_mb_cmp.size );
  const Cmp::Position plant_pos_cmp( plant_mb_cmp.position, Constants::kGridSizePxF );

  // drop one random item of the destroy_action reward (drop, forage or the plant itself, see items.json) beside the
  // plant. It is a reward for the player, so it may be picked up straight into an empty slot (items.json "auto_pickup").
  // Plants without a destroy_action.reward.item drop nothing.
  // A dropped plant item (e.g. the plant itself) lies on its side: its 1x2 sprite is rotated 90 degrees about the centre
  // of its top tile, so the top stays on the drop cell and the base lies in the cell to its left. Replanting builds a
  // fresh, unrotated multiblock (see InventorySystem), so it stands upright again.
  const auto lay_plant_on_side = [this]( entt::entity item_entt )
  {
    const auto *item_cmp = reg().try_get<Cmp::WorldItem>( item_entt );
    if ( not item_cmp or not item_cmp->item_type.str().starts_with( "item.plant." ) ) return;
    reg().emplace_or_replace<Cmp::AbsoluteRotation>( item_entt, 90.f );
    reg().emplace_or_replace<Cmp::AbsoluteOffset>( item_entt, Constants::kGridSizePxF / 2.f );
  };
  if ( destroy_reward and destroy_reward->target == Cmp::ActionReward::Target::DEFAULT )
  {
    get_systems_event_queue().trigger( Events::CreateItemEvent( Cmp::Position( find_drop_pos( plant_mb_cmp ), Constants::kGridSizePxF ),
                                                                Sys::ItemStore::instance().pick_reward_item( *destroy_reward ), "drop_loot", 0.f,
                                                                false, 0, entt::null, lay_plant_on_side, /*sting=*/true, /*player_reward=*/true ) );
  }
  // the weapon occupies the inventory, so an inventory reward is dropped nearby
  else if ( destroy_reward ) { grant_item_reward( *destroy_reward, plant_bounds, plant_entt, lay_plant_on_side ); }
  const auto destroy_action = Utils::Player::apply_action_from_world_item<Cmp::DestroyAction>( reg(), plant_entt );

  // the NPCs the plant spawned while alive die with it; its loot is left for the player to collect. Done before the
  // destroy_action side effects, otherwise an NPC they spawn (e.g. a tick from grass) is linked to the plant and dies at once
  destroy_plant_spawned_npcs( plant_uuid_cmp, Sys::SpriteKey( "sprite.death.anim.bloodsplat" ) );
  trigger_side_effects( destroy_action, plant_pos_cmp, plant_entt );
  get_systems_event_queue().trigger( Events::DestroyPlantEvent( plant_entt ) );
  play_action_sfx( destroy_action );
}

void PlantSystem::try_burn_plant()
{
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type == "item.torch" )
  {
    for ( auto [plant_entt, plant_cmp] : reg().view<Cmp::PlantMultiBlock>().each() )
    {
      if ( not Utils::Player::is_player_near( reg(), plant_cmp ) ) continue;
      if ( not Utils::Player::get_projected_position( reg() ).findIntersection( plant_cmp ) ) continue;

      if ( not reg().any_of<Cmp::Plant::BurningTimer>( plant_entt ) ) { reg().emplace_or_replace<Cmp::Plant::BurningTimer>( plant_entt ); }
    }
  }
}

void PlantSystem::update_burning_plants( sf::Time dt )
{
  static const std::string kAshPileTag = "graveyard.plant.particle.ash";
  static const std::string kPlantFireTag = "graveyard.plant.particle.flame";
  static const std::string kPlantSmokeTag = "graveyard.plant.particle.smoke";

  // negative burn time (-1) means plants burn forever
  const float burn_time = Sys::PersistSystem::get<Cmp::Persist::PlantBurnTime>( reg() ).get_value();
  const bool burns_forever = burn_time < 0.f;

  for ( auto [plant_entt, plant_cmp, plant_uuid] : reg().view<Cmp::PlantMultiBlock, Cmp::UUID>().each() )
  {
    auto *burn_timer = reg().try_get<Cmp::Plant::BurningTimer>( plant_entt );
    if ( not burn_timer ) continue;

    // burn_action.sfx: "loop" plays while burning, "final" once burnt out (see items.json). Empty if the item has no
    // burn_action, or the plant entity has no Cmp::WorldItem.
    Cmp::ActionSfx burn_sfx;
    if ( const auto *burn_item = reg().try_get<Cmp::WorldItem>( plant_entt ) ) { burn_sfx = burn_item->get_action<Cmp::BurnAction>().sfx(); }

    if ( burns_forever or *burn_timer < sf::seconds( burn_time ) )
    {
      // update player stats with burn_action: apply once if interval == 0, otherwise every `interval`
      // seconds. BurningTimeAccumulator resets to zero at the start of each burn, so a one-shot fires
      // only on that very first frame, and a repeating tick fires whenever this frame's dt carries the
      // accumulator across a new multiple of interval.
      auto *plant_item = reg().try_get<Cmp::WorldItem>( plant_entt );
      if ( plant_item and Utils::Player::is_player_near( reg(), plant_cmp ) )
      {
        auto burn_action = plant_item->get_action<Cmp::BurnAction>();
        auto interval = burn_action.interval();
        bool should_apply_burn_action;
        if ( interval <= 0.f ) { should_apply_burn_action = ( *burn_timer == sf::Time::Zero ); }
        else
        {
          auto elapsed_before = burn_timer->asSeconds();
          auto elapsed_after = elapsed_before + dt.asSeconds();
          should_apply_burn_action = static_cast<int>( elapsed_before / interval ) != static_cast<int>( elapsed_after / interval );
        }

        if ( should_apply_burn_action ) { Utils::Player::apply_action_from_world_item<Cmp::BurnAction>( reg(), plant_entt ); }
      }

      // stll burning
      play_action_loop_sfx( burn_sfx );

      // don't create a duplicate particle sprite if this plant already has a flame
      bool already_has_flame = false;
      for ( auto [ps_owner_entt, ps_owner_cmp, ps_owner_uuid] : reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
      {
        if ( ps_owner_uuid == plant_uuid )
        {
          already_has_flame = true;
          break;
        }
      }
      if ( not already_has_flame )
      {
        const sf::Vector2f flame_emitter_pos( plant_cmp.getCenter().x, plant_cmp.position.y + plant_cmp.size.y - 4.f );
        constexpr auto ps_scale = 0.5f;
        constexpr auto particle_size = 3.f;
        constexpr auto particle_speed = 60.f;
        constexpr auto particle_lifetime = 2.f;
        constexpr auto particle_count = 600;
        Factory::Particle::add_flame( reg(), kPlantFireTag, plant_uuid, flame_emitter_pos, plant_cmp.position.y + Constants::kGridSizePxF.y, ps_scale,
                                      particle_size, particle_speed, particle_lifetime, particle_count );

        const sf::Vector2f ash_emitter_pos( plant_cmp.position.x, plant_cmp.getCenter().y + 8 );
        constexpr auto ash_scale = 1.f;
        constexpr auto ash_particle_size = 1.f;
        constexpr auto ash_particle_speed = 20.f;
        constexpr auto ash_particle_count = 1000;

        // Tag with plant_uuid, same as the flame, so both are found via the loops below. Unlike the
        // flame, the ash pile is meant to outlive the plant (it's a permanent mark on the ground), so
        // the "all done" branch below stops it rather than destroying it, distinguishing it from the
        // flame by tag rather than by giving it a separate identity.
        Factory::Particle::add_ashpile( reg(), kAshPileTag, plant_uuid, ash_emitter_pos, plant_cmp.position.y + 1, ash_scale, ash_particle_size,
                                        ash_particle_speed, ash_particle_count );
      }

      // burning yew trees throw off embers. Skip trees that are off screen: their particle sprites
      // aren't simulated, so the embers would pile up unlaunched.
      if ( ( plant_item != nullptr ) and plant_item->item_type == "item.plant.yew" and
           Utils::is_visible_in_view( RenderSystem::get_world_view(), plant_cmp ) )
      {
        constexpr float kEmberInterval = 0.4f;
        const float elapsed_before = burn_timer->asSeconds();
        const float elapsed_after = elapsed_before + dt.asSeconds();
        if ( static_cast<int>( elapsed_before / kEmberInterval ) != static_cast<int>( elapsed_after / kEmberInterval ) )
        {
          // each burst needs its own UUID: particle sprites sharing the plant's UUID are treated as its
          // flame, and are destroyed when the plant has finished burning
          auto ember_uuid = Cmp::UUID::generate();
          const sf::Vector2f ember_emitter_pos( plant_cmp.getCenter().x, plant_cmp.position.y + plant_cmp.size.y - 4.f );
          Factory::Particle::add_ember_ps( reg(), std::string( Cmp::Particle::EmberParticleSprite::kYewTag ), 2, 2.f, 140.f, 1.5f, ember_uuid,
                                           ember_emitter_pos, plant_cmp.position.y + Constants::kGridSizePxF.y + 1.f );
        }
      }

      *burn_timer += dt;
    }
    else
    {
      // all done
      stop_action_loop_sfx( burn_sfx );
      play_action_sfx( burn_sfx );
      reg().remove<Cmp::Plant::BurningTimer>( plant_entt );
      for ( auto [ps_owner_entt, ps_owner_cmp, ps_owner_uuid] : reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
      {
        if ( ps_owner_uuid != plant_uuid ) continue;

        // the ash pile outlives the plant by design (it's a permanent mark on the ground), so stop it
        // instead of destroying it along with the plant's other particle sprites
        if ( ps_owner_cmp.sprite->get_tag() == kAshPileTag ) { ps_owner_cmp.sprite->stop(); }
        else { reg().destroy( ps_owner_entt ); }
      }
      const sf::Vector2f emitter_pos( plant_cmp.getCenter().x, plant_cmp.position.y + ( plant_cmp.size.y - 7 ) );
      auto burnt_uuid = Cmp::UUID::generate();
      Factory::Particle::add_smoke( reg(), kPlantSmokeTag, burnt_uuid, emitter_pos, plant_cmp.position.y );

      // burn_action side effects fire once when the plant has burnt out, whether or not the player is near
      if ( auto *plant_item = reg().try_get<Cmp::WorldItem>( plant_entt ) )
      {
        const auto burn_action = plant_item->get_action<Cmp::BurnAction>();
        if ( not burn_action.side_effects().empty() )
        {
          get_systems_event_queue().trigger( Events::GrimoireItemUpdateEvent( plant_item->item_type, Cmp::BurnAction::kKey, burn_action ) );
          trigger_side_effects( burn_action, Cmp::Position( plant_cmp.position, Constants::kGridSizePxF ), plant_entt );
        }
      }
      destroy_plant_spawned_npcs( plant_uuid, Sys::SpriteKey( "sprite.death.anim.banish" ) );
      destroy_plant_spawned_loot( plant_uuid );
      get_systems_event_queue().trigger( Events::DestroyPlantEvent( plant_entt ) );
    }
  }
}

void PlantSystem::destroy_plant_spawned_npcs( const Cmp::UUID &plant_uuid, const Sys::SpriteKey &death_anim )
{
  // collect first: the event handler modifies this storage
  std::vector<entt::entity> npcs;
  for ( auto [npc_entt, npc_cmp, spawned_by_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Npc::SpawnedBy>().each() )
  {
    if ( spawned_by_cmp.m_source == plant_uuid ) npcs.push_back( npc_entt );
  }
  for ( auto npc_entt : npcs )
  {
    get_systems_event_queue().trigger( Events::DestroyNpcEvent( npc_entt, death_anim ) );
  }
}

void PlantSystem::destroy_plant_spawned_loot( const Cmp::UUID &plant_uuid )
{
  // collect first: the event handler modifies this storage
  std::vector<entt::entity> loot;
  for ( auto [loot_entt, loot_cmp, dropped_by_cmp] : reg().view<Cmp::Loot, Cmp::DroppedBy>().each() )
  {
    if ( dropped_by_cmp.m_source == plant_uuid ) loot.push_back( loot_entt );
  }
  for ( auto loot_entt : loot )
  {
    get_systems_event_queue().trigger( Events::DestroyLootEvent( loot_entt ) );
  }
}

sf::Vector2f PlantSystem::find_drop_pos( const Cmp::PlantMultiBlock &plant_mb_cmp )
{
  const auto grid = Constants::kGridSizePxF;
  const auto plant_pos = Utils::snap_to_grid( plant_mb_cmp.position );

  // the plant's head cell is the top row of its footprint (middle column). The player is blocked from the base cell, so
  // they couldn't pick up anything dropped there.
  const int cols = static_cast<int>( std::round( plant_mb_cmp.size.x / grid.x ) );
  const sf::Vector2f head_cell( plant_pos.x + static_cast<float>( cols / 2 ) * grid.x, plant_pos.y );

  auto npc_navmesh = m_npc_navmesh.lock();
  auto ghost_navmesh = m_ghost_navmesh.lock();
  auto loot_sm = m_loot_sm.lock();
  auto reserved_sm = m_reserved_sm.lock();
  const Utils::SpawnGrids grids{ npc_navmesh.get(), ghost_navmesh.get(), loot_sm.get(), reserved_sm.get(), m_item_sm.lock().get() };

  // one ring around the plant footprint, picked at random
  auto cell = Utils::find_random_cell_near( plant_mb_cmp, 1, [&]( const sf::Vector2f &cell_pos )
  { return Utils::is_free_spawn_area( reg(), sf::FloatRect( cell_pos, grid ), grids ); } );
  // no free cell around the plant so overlap the plant's head, where the player can still reach it
  return cell.value_or( head_cell );
}

} // namespace Game::Sys
