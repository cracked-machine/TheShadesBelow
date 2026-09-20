#include <Audio/SoundBank.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/Inventory/FlashUICadaver.hpp>
#include <Components/Inventory/FlashUIRadius.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Persistent/BombBonus.hpp>
#include <Components/Persistent/HealthBonus.hpp>
#include <Components/Player/BlastRadius.hpp>
#include <Components/Player/CadaverCount.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/CryptRoomEvent.hpp>
#include <Events/UnlockDoorEvent.hpp>
#include <Factory/LootFactory.hpp>
#include <Systems/LootSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>

namespace Game::Sys
{

LootSystem::LootSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank )
{
  SPDLOG_DEBUG( "LootSystem initialized" );
}

void LootSystem::check_loot_collision()
{
  // Store both loot effects and the player velocities
  struct LootEffect
  {
    entt::entity loot_entity;
    Sprites::SpriteMetaType type;
    entt::entity player_entity;
  };

  std::vector<LootEffect> loot_effects;
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [pc_entt, pc_cmp, pc_pos_cmp] : reg().view<Cmp::Player::Character, Cmp::Position>().each() )
  {
    // reduce hitbox to prevent premature pickup
    auto player_hitbox = Cmp::RectBounds::scaled( pc_pos_cmp.position, pc_pos_cmp.size, 0.5f );
    for ( auto [loot_entt, loot_cmp, loot_pos_cmp, loot_sprite_anim] : reg().view<Cmp::Loot, Cmp::Position, Cmp::AnimData>().each() )
    {
      if ( not Utils::is_visible_in_view( view_bounds, loot_pos_cmp ) ) continue;

      if ( pc_pos_cmp.findIntersection( loot_pos_cmp ) )
      {
        // Store effect to apply after collision detection
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
      wealth_cmp->wealth += 1;
      collect_loot( effect.loot_entity );
    }
    else { SPDLOG_WARN( "Unknown loot type encountered during pickup: {}", effect.type ); }
  }
}

} // namespace Game::Sys