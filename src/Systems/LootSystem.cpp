#include <Audio/SoundBank.hpp>
#include <Components/AnimData.hpp>
#include <Components/Inventory/FlashUICadaver.hpp>
#include <Components/Inventory/FlashUIHealth.hpp>
#include <Components/Inventory/FlashUIInventory.hpp>
#include <Components/Inventory/FlashUIRadius.hpp>
#include <Components/Inventory/FlashUIWealth.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Loot.hpp>
#include <Components/LootContainer.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Persistent/HealthBonus.hpp>
#include <Components/Player/BlastRadius.hpp>
#include <Components/Player/CadaverCount.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/UUID.hpp>
#include <Events/CryptRoomEvent.hpp>
#include <Factory/LootFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/LootSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

namespace Game::Sys
{

LootSystem::LootSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue()
                    .sink<Events::DestroyLootContainerEvent>()
                    .connect<&LootSystem::on_destroy_loot_container_event>( this );
  SPDLOG_DEBUG( "LootSystem initialized" );
}

void LootSystem::update_loot_container_particles()
{
  Factory::Particle::delete_expired_particle_sprites( reg(), "graveyard.pot.particle.dig" );
}

void LootSystem::on_destroy_loot_container_event( const Events::DestroyLootContainerEvent &event )
{
  auto loot_entity = event.m_loot_container_entity;
  // already broken, e.g. hit by two arrows or caught in overlapping blasts in the same tick
  if ( not reg().valid( loot_entity ) or not reg().all_of<Cmp::LootContainer, Cmp::Position>( loot_entity ) ) return;

  // copy the position: destroy_loot_container removes components from this entity
  Cmp::Position destroyed_pos_cmp = reg().get<Cmp::Position>( loot_entity );
  auto reserved_sm = m_reserved_sm.lock();

  m_sound_bank.get_effect( "break_pot" ).play();
  Factory::Loot::destroy_loot_container( reg(), loot_entity, reserved_sm );

  // add the position to the spatial grid so it can be used in pathfinding
  if ( PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = m_npc_navmesh.lock() ) pathfinding_navmesh->insert( loot_entity, destroyed_pos_cmp );
  if ( PathFinding::SpatialHashGridSharedPtr ghost_navmesh = m_ghost_navmesh.lock() ) ghost_navmesh->insert( loot_entity, destroyed_pos_cmp );
  if ( PathFinding::SpatialHashGridSharedPtr player_navmesh = m_player_navmesh.lock() ) player_navmesh->remove( loot_entity, destroyed_pos_cmp );

  auto pot_particle_uuid = Cmp::UUID::generate();
  Factory::Particle::add_pot_ps( reg(), "graveyard.pot.particle.dig", 20, 2.f, 50.f, 10.f, pot_particle_uuid, destroyed_pos_cmp.position,
                                 destroyed_pos_cmp.position.y );

  // drop loot - 50% chance
  auto loot_choices = std::vector<Sys::SpriteKey>{ "sprite.graveyard.loot.health", "sprite.graveyard.loot.blast", "sprite.graveyard.loot.repair" };
  const auto &loot_ss = Sys::SpriteStore::instance().get_random( loot_choices );
  Cmp::RandomInt do_drop( 0, 1 );
  if ( do_drop.gen() == 0 )
  {
    // clang-format off
    auto dropped_loot_entt = Factory::Loot::create_loot_drop(
        reg(),
        Cmp::AnimData(Cmp::AnimData::Config{ 
          .sprite_type = loot_ss.type(), 
          .frame_index_offset = loot_ss.get_random_texture_index(), 
          .enabled = false 
        } ),
        sf::FloatRect( Utils::snap_to_grid( destroyed_pos_cmp.position ), Constants::kGridSizePxF ), 
        Factory::IncludePack<>{},
        Factory::ExcludePack<Cmp::Player::Character, Cmp::Obstacle>{}, 
        // no spatial exclude for the player: they may be adjacent to the container's tile when it breaks
        Factory::ExcludePack<Cmp::Obstacle>{},
        /*zorder_offset=*/-8.f, 
        reserved_sm.get() 
    );
    // clang-format on
    if ( dropped_loot_entt != entt::null )
    {
      SPDLOG_DEBUG( "Loot was dropped at {},{}", destroyed_pos_cmp.position.x, destroyed_pos_cmp.position.y );
      m_sound_bank.get_effect( "drop_loot" ).play();
    }
  }
}

void LootSystem::check_loot_collision( sf::Time dt )
{
  // Store both loot effects and the player velocities
  struct LootEffect
  {
    entt::entity loot_entity;
    Sys::SpriteKey type;
    entt::entity player_entity;
  };

  std::vector<LootEffect> loot_effects;
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [pc_entt, pc_cmp, pc_pos_cmp] : reg().view<Cmp::Player::Character, const Cmp::Position>().each() )
  {
    // reduce hitbox to prevent premature pickup
    auto player_hitbox = Cmp::RectBounds::scaled( pc_pos_cmp.position, pc_pos_cmp.size, 0.5f );
    for ( auto [loot_entt, loot_cmp, loot_pos_cmp, loot_sprite_anim] : reg().view<Cmp::Loot, Cmp::Position, Cmp::AnimData>().each() )
    {
      // always update the loot timer, even off screen loot
      loot_cmp.cooldown_timer += dt;

      // only process on-screen loot
      if ( not Utils::is_visible_in_view( view_bounds, loot_pos_cmp ) ) continue;
      if ( pc_pos_cmp.findIntersection( loot_pos_cmp ) and loot_cmp.cooldown_timer >= loot_cmp.timeout() )
      {
        // Store effect to apply after collision detection. Don't bother reseting the cmp timer, we're about to destroy the cmp.
        loot_effects.push_back( { loot_entt, loot_sprite_anim.m_sprite_type, pc_entt } );
      }
    }
  }

  // collects the loot: plays the pickup sound and removes the loot entity
  auto collect_loot = [this]( entt::entity loot_entity )
  {
    m_sound_bank.get_effect( "get_loot" ).play();
    Factory::Loot::destroy_loot_drop( reg(), loot_entity );
  };

  // Second pass: apply effects and remove loots
  for ( const auto &effect : loot_effects )
  {
    if ( !reg().valid( effect.player_entity ) ) continue;

    // Apply the effect
    if ( effect.type == "sprite.graveyard.loot.health" )
    {
      auto &health_bonus = Sys::PersistSystem::get<Cmp::Persist::HealthBonus>( reg() );

      // signal UI to flash
      auto flash_entt = reg().create();
      reg().emplace_or_replace<Cmp::FlashUIHealth>( flash_entt );

      Utils::Player::get_stats( reg() ).apply( { Cmp::Stats::Health{ health_bonus.get_value() }, {}, {}, {}, {}, {} } );
      collect_loot( effect.loot_entity );
    }
    else if ( effect.type == "sprite.graveyard.loot.repair" )
    {
      // update the wear level of the first matching tool in the player inventory, if any
      auto inventory_view = reg().view<Cmp::PlayerInventorySlot>();
      for ( auto [weapons_entity, inventory_slot] : inventory_view.each() )
      {
        if ( inventory_slot.m_item.sprite_type.contains( "axe" ) or inventory_slot.m_item.sprite_type.contains( "pickaxe" ) or
             inventory_slot.m_item.sprite_type.contains( "shovel" ) )
        {
          auto *wear_level_cmp = reg().try_get<Cmp::Inventory::WearLevel>( weapons_entity );
          if ( wear_level_cmp )
          {
            // increase weapon level by 50, up to max level 100
            wear_level_cmp->m_level = std::clamp( wear_level_cmp->m_level + 50.f, 0.f, 100.f );
            collect_loot( effect.loot_entity );

            // signal UI to flash
            auto flash_entt = reg().create();
            reg().emplace_or_replace<Cmp::FlashUIInventory>( flash_entt );

            break; // only repair (and consume) one tool per loot pickup
          }
        }
      }
    }
    else if ( effect.type == "sprite.graveyard.loot.blast" )
    {
      auto *blast_radius = reg().try_get<Cmp::Player::BlastRadius>( effect.player_entity );
      if ( not blast_radius )
      {
        SPDLOG_WARN( "Player entt has no component: Cmp::Player::BlastRadius" );
        continue;
      }
      blast_radius->value = std::clamp( blast_radius->value + 1, 0, 5 );
      collect_loot( effect.loot_entity );

      // signal UI to flash
      auto flash_entt = reg().create();
      reg().emplace_or_replace<Cmp::FlashUIRadius>( flash_entt );
    }
    else if ( effect.type == "sprite.crypt.loot.cadaver" )
    {
      auto *pc_cadaver_count = reg().try_get<Cmp::Player::CadaverCount>( effect.player_entity );
      if ( not pc_cadaver_count )
      {
        SPDLOG_WARN( "Player entt has no component: Cmp::Player::CadaverCount" );
        continue;
      }
      pc_cadaver_count->increment_count( 1 );
      collect_loot( effect.loot_entity );
      m_sound_bank.get_effect( "secret" ).play();

      // signal UI to flash
      auto flash_entt = reg().create();
      reg().emplace_or_replace<Cmp::FlashUICadaver>( flash_entt );

      Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, Cmp::Stats::Infamy{ 30 }, {}, {} } );

      get_systems_event_queue().trigger( Events::CryptRoomEvent( Events::CryptRoomEvent::Type::EXIT_ALL_PASSAGES ) );
    }
    else if ( effect.type == "sprite.crypt.loot.gold" )
    {
      auto *wealth_cmp = reg().try_get<Cmp::Player::Wealth>( effect.player_entity );
      if ( not wealth_cmp )
      {
        SPDLOG_WARN( "Player entt has no component: Cmp::Player::Wealth" );
        continue;
      }
      // signal UI to flash
      auto flash_entt = reg().create();
      reg().emplace_or_replace<Cmp::FlashUIWealth>( flash_entt );

      wealth_cmp->wealth += 1;
      collect_loot( effect.loot_entity );
    }
    else { SPDLOG_WARN( "Unknown loot type encountered during pickup: {}", effect.type ); }
  }
}

} // namespace Game::Sys