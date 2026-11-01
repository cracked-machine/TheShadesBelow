#include <Audio/SoundBank.hpp>
#include <Components/DroppedBy.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Loot.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/SpawnedBy.hpp>
#include <Components/Particle/EmberParticleSprite.hpp>
#include <Components/Persistent/GlovesWearPerForage.hpp>
#include <Components/Persistent/PlantWearPerForage.hpp>
#include <Components/Plant/BurningTimer.hpp>
#include <Components/Player/DiggingTimer.hpp>
#include <Components/Position.hpp>
#include <Components/SelectedPosition.hpp>
#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ForageAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Components/UUID.hpp>
#include <Events/AddInventoryEvent.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/CreatePlantEvent.hpp>
#include <Events/DestroyLootEvent.hpp>
#include <Events/DestroyNpcEvent.hpp>
#include <Events/DestroyPlantEvent.hpp>
#include <Events/DropInventoryEvent.hpp>
#include <Events/GrimoireUpdateEvent.hpp>
#include <Events/PickupWorldItemEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PlantFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/ParticleSystem.hpp>
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
  auto reserved_sm = m_reserved_sm.lock();

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
    for ( auto tile_entt : reserved_sm->at( seg_pos_cmp ) )
    {
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

  const auto &plant_ss = Sys::SpriteStore::instance().get( Sys::SpriteKey( "sprite." + plant_type.str() ) );

  // Plants can span more than one grid row (e.g. 1x2), so every cell in the footprint - not
  // just the origin - must be a real, unreserved world tile. Otherwise a plant can overlap a
  // reserved tile it doesn't share an entity with, e.g. the player spawn.
  auto plant_grid_size = plant_ss.get_grid_size();
  bool footprint_clear = true;
  for ( int gy = 0; gy < plant_grid_size.y and footprint_clear; ++gy )
  {
    for ( int gx = 0; gx < plant_grid_size.x; ++gx )
    {
      sf::Vector2f pos_offset( static_cast<float>( gx ) * Constants::kGridSizePxF.x, static_cast<float>( gy ) * Constants::kGridSizePxF.y );
      Cmp::Position cell_pos( pos + pos_offset, Constants::kGridSizePxF );
      if ( Utils::get_world_pos_entt( reg(), cell_pos ) == entt::null or not reserved_sm->at( cell_pos ).empty() )
      {
        footprint_clear = false;
        break;
      }
    }
  }

  if ( footprint_clear )
  {
    // now create the plant at a new entt
    auto [mb_entt, _] = Factory::Multiblock::add_multiblock_with_segments<Cmp::PlantMultiBlock, Cmp::PlantSegment>( reg(), pos, plant_ss, 0, 0,
                                                                                                                    reserved_sm.get() );

    // Add the worlditem now so we don't have to look it up later when digging up the plant
    const auto &plant_item = Sys::ItemStore::instance().get( plant_type );
    reg().emplace_or_replace<Cmp::WorldItem>( mb_entt, plant_item );
    // the wear level limits how many items can be foraged from the plant
    if ( plant_item.has_wear() ) { reg().emplace_or_replace<Cmp::Inventory::WearLevel>( mb_entt, 100.f ); }

    // Protect every cell covered by this plant: segment entities carry the UUID and
    // are at each tile position. Insert them so subsequent placement calls see them
    // as reserved via the O(1) navmesh check.
    auto *mb_uuid = reg().try_get<Cmp::UUID>( mb_entt );
    if ( mb_uuid )
    {
      for ( auto [seg_entt, seg_cmp, seg_pos, seg_uuid] : reg().view<Cmp::PlantSegment, Cmp::Position, Cmp::UUID>().each() )
      {
        if ( seg_uuid == *mb_uuid ) reserved_sm->insert( seg_entt, seg_pos );
      }
    }
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

      auto *plant_item = reg().try_get<Cmp::WorldItem>( plant_entt );
      if ( not plant_item ) continue;

      auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );

      // copy the type and rewards: creating items or removing the plant below can invalidate plant_item
      const auto plant_type = plant_item->item_type;
      const auto forage_reward = plant_item->get_reward<Cmp::ForageAction>();
      const auto axe_reward = plant_item->get_reward<Cmp::DestroyAction>();
      const auto axe_hits = plant_item->get_axe_hits();
      const Cmp::Position plant_bounds( plant_mb_cmp.position, plant_mb_cmp.size );

      // each forage wears the plant down
      auto wear_plant = [&]()
      {
        auto wear = Sys::PersistSystem::get<Cmp::Persist::PlantWearPerForage>( reg() ).get_value();
        get_systems_event_queue().trigger( Events::UpdateDmgEvent( plant_entt, wear ) );
      };
      // a fully foraged plant withers away - no drops
      // forage_action is optional (see items.json). With gloves only its side effects apply, not its stat changes.
      const Cmp::Position plant_pos_cmp( plant_mb_cmp.position, Constants::kGridSizePxF );
      auto apply_forage_action = [&]( bool apply_stats )
      {
        const auto &world_item = reg().get<Cmp::WorldItem>( plant_entt );
        if ( not world_item.has_action<Cmp::ForageAction>() ) return;
        const auto forage_action = apply_stats ? Utils::Player::apply_action_from_world_item<Cmp::ForageAction>( reg(), plant_entt )
                                               : world_item.get_action<Cmp::ForageAction>();
        trigger_side_effects( forage_action, plant_pos_cmp, plant_entt );
      };

      auto remove_if_depleted = [&]()
      {
        if ( not Utils::is_plant_depleted( reg(), plant_entt ) ) return;
        get_systems_event_queue().trigger( Events::DestroyPlantEvent( plant_entt ) );
      };

      if ( inventory_type == "" )
      {
        // plants without a forage_action.reward.item (see items.json) give nothing
        if ( forage_reward )
        {
          auto plantleaves_particle_uuid = Cmp::UUID::generate();
          Factory::Particle::add_plantleaves_ps( reg(), "graveyard.plant.particle.leaves", 50, 2.f, 50.f, 14.f, plantleaves_particle_uuid,
                                                 plant_mb_cmp.getCenter(), plant_mb_cmp.position.y );
          if ( forage_reward->target == Cmp::ActionReward::Target::DEFAULT )
          {
            get_systems_event_queue().trigger( Events::AddInventoryEvent( Sys::ItemStore::instance().pick_item( forage_reward->item ) ) );
          }
          else { grant_item_reward( *forage_reward, plant_bounds, plant_entt ); }
          // only an item that went to the inventory has its spawn_action applied here
          if ( forage_reward->target != Cmp::ActionReward::Target::NEARBY )
          {
            const auto spawn_action = Utils::Player::apply_action_from_inventory_item<Cmp::SpawnAction>( reg() );
            trigger_side_effects( spawn_action, Utils::Player::get_position( reg() ) );
          }
          m_sound_bank.get_effect( "chopping_final" ).play();
          wear_plant();
        }
        apply_forage_action( true );
        remove_if_depleted();
      }
      else if ( inventory_type == "item.gloves" )
      {
        // Gloves stay equipped: the forage is dropped next to the plant, marked safe so its spawn/carry actions never
        // sting the player, and the plant's action modifiers are deliberately skipped. They don't protect against
        // hawthorn lightning though.
        if ( ( plant_type == "item.plant.nettle" or plant_type == "item.plant.hawthorn" ) and forage_reward )
        {
          auto plantleaves_particle_uuid = Cmp::UUID::generate();
          Factory::Particle::add_plantleaves_ps( reg(), "graveyard.plant.particle.leaves", 50, 2.f, 50.f, 14.f, plantleaves_particle_uuid,
                                                 plant_mb_cmp.getCenter(), plant_mb_cmp.position.y );
          if ( forage_reward->target == Cmp::ActionReward::Target::DEFAULT )
          {
            get_systems_event_queue().trigger(
                Events::CreateItemEvent( Cmp::Position( find_forage_drop_pos( plant_mb_cmp ), Constants::kGridSizePxF ),
                                         Sys::ItemStore::instance().pick_item( forage_reward->item ), "drop_loot", 0.f, false, 0, entt::null,
                                         [this]( entt::entity item_entt ) { reg().get<Cmp::WorldItem>( item_entt ).set_safe(); } ) );
          }
          // the gloves occupy the inventory, so an inventory reward is dropped nearby
          else { grant_item_reward( *forage_reward, plant_bounds, plant_entt ); }
          wear_plant();
          auto gloves_wear = Sys::PersistSystem::get<Cmp::Persist::GlovesWearPerForage>( reg() ).get_value();
          get_systems_event_queue().trigger( Events::UpdateDmgEvent( Utils::Player::get_inventory_entt( reg() ), gloves_wear ) );
          apply_forage_action( false );
          remove_if_depleted();
        }
      }
      else
      {
        if ( Utils::Player::get_inventory_wear_level( reg() ) <= 0 ) { return; }

        // Prevent digging/chopping a plant on fire
        auto *burning_accum = reg().try_get<Cmp::Plant::BurningTimer>( plant_entt );
        if ( burning_accum ) continue;

        if ( inventory_type == "item.shovel" )
        {
          // damage the shovel before it is dropped
          get_systems_event_queue().trigger(
              Events::UpdateDmgEvent( Utils::Player::get_inventory_entt( reg() ), static_cast<float>( plant_item->get_shovel_damage() ) ) );

          // dig up the plant
          get_systems_event_queue().trigger( Events::DropInventoryEvent() );
          // spawn_action side effects are triggered by InventorySystem when it handles the pickup
          Utils::Player::apply_action_from_world_item<Cmp::SpawnAction>( reg(), plant_entt );
          get_systems_event_queue().trigger( Events::PickupWorldItemEvent( plant_entt ) );
          m_sound_bank.get_effect( "digging_earth" ).play();
        }
        else if ( inventory_type == "item.axe" )
        {
          // destroy the plant
          auto plantleaves_particle_uuid = Cmp::UUID::generate();
          Factory::Particle::add_plantleaves_ps( reg(), "graveyard.plant.particle.leaves", 50, 2.f, 50.f, 14.f, plantleaves_particle_uuid,
                                                 plant_mb_cmp.getCenter(), plant_mb_cmp.position.y );
          auto planttwigs_particle_uuid = Cmp::UUID::generate();
          Factory::Particle::add_planttwigs_ps( reg(), "graveyard.plant.particle.twigs", 10, 2.f, 50.f, 14.f, planttwigs_particle_uuid,
                                                plant_mb_cmp.getCenter(), plant_mb_cmp.position.y );

          // damage the axe before it is dropped
          get_systems_event_queue().trigger(
              Events::UpdateDmgEvent( Utils::Player::get_inventory_entt( reg() ), static_cast<float>( plant_item->get_axe_damage() ) ) );

          // tougher plants (axe_hits in items.json) lose 1/axe_hits of their wear level per hit, rounded up so exactly
          // axe_hits hits fell an unforaged plant
          if ( axe_hits > 1 )
          {
            get_systems_event_queue().trigger( Events::UpdateDmgEvent( plant_entt, std::ceil( 100.f / static_cast<float>( axe_hits ) ) ) );
            if ( not Utils::is_plant_depleted( reg(), plant_entt ) )
            {
              m_sound_bank.get_effect( "chopping" ).play();
              continue;
            }
          }

          // plants without a destroy_action.reward.item (see items.json) drop nothing
          if ( axe_reward and axe_reward->target == Cmp::ActionReward::Target::DEFAULT )
          {
            get_systems_event_queue().trigger( Events::CreateItemEvent( Cmp::Position( plant_mb_cmp.position, Constants::kGridSizePxF ),
                                                                        Sys::ItemStore::instance().pick_item( axe_reward->item ), "drop_loot" ) );
          }
          // the axe occupies the inventory, so an inventory reward is dropped nearby
          else if ( axe_reward ) { grant_item_reward( *axe_reward, plant_bounds, plant_entt ); }
          const auto destroy_action = Utils::Player::apply_action_from_world_item<Cmp::DestroyAction>( reg(), plant_entt );

          trigger_side_effects( destroy_action, plant_pos_cmp, plant_entt );
          // its loot is left for the player to collect
          destroy_plant_spawned_npcs( plant_uuid_cmp, Sys::SpriteKey( "sprite.death.anim.bloodsplat" ) );
          get_systems_event_queue().trigger( Events::DestroyPlantEvent( plant_entt ) );
          m_sound_bank.get_effect( "chopping_final" ).play();
        }
      }
    }
  }
}

void PlantSystem::try_burn_plant()
{
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type == "item.candle" )
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

  for ( auto [plant_entt, plant_cmp, plant_uuid] : reg().view<Cmp::PlantMultiBlock, Cmp::UUID>().each() )
  {
    auto *burn_timer = reg().try_get<Cmp::Plant::BurningTimer>( plant_entt );
    if ( not burn_timer ) continue;

    if ( *burn_timer < burn_timer->timeout() )
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
      if ( m_sound_bank.get_effect( "burning" ).getStatus() != sf::Sound::Status::Playing ) { m_sound_bank.get_effect( "burning" ).play(); }
      m_sound_bank.get_effect( "burning" ).setLooping( false );

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
      m_sound_bank.get_effect( "burning" ).stop();
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
          get_systems_event_queue().trigger( Events::GrimoireUpdateEvent( plant_item->item_type, Cmp::BurnAction::kKey, burn_action ) );
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

sf::Vector2f PlantSystem::find_forage_drop_pos( const Cmp::PlantMultiBlock &plant_mb_cmp )
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
  const Utils::SpawnGrids grids{ npc_navmesh.get(), ghost_navmesh.get(), loot_sm.get(), reserved_sm.get() };

  // one ring around the plant footprint, picked at random
  auto cell = Utils::find_random_cell_near( plant_mb_cmp, 1, [&]( const sf::Vector2f &cell_pos )
  { return Utils::is_free_spawn_area( reg(), sf::FloatRect( cell_pos, grid ), grids ); } );
  // no free cell around the plant so overlap the plant's head, where the player can still reach it
  return cell.value_or( head_cell );
}

} // namespace Game::Sys
