#include <Components/AbsoluteAlpha.hpp>
#include <Components/AbsoluteOffset.hpp>
#include <Components/AbsoluteRenderOffset.hpp>
#include <Components/AbsoluteRotation.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/AnimData.hpp>
#include <Components/DeathPosition.hpp>
#include <Components/Direction.hpp>
#include <Components/Grave/CurseTablet.hpp>
#include <Components/Inventory/DowsingTarget.hpp>
#include <Components/Inventory/FlashUIExtraLife.hpp>
#include <Components/Inventory/FlashUIHealth.hpp>
#include <Components/Inventory/Grimoire.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/StingDamage.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/LastDirection.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/SummonOffering.hpp>
#include <Components/Particle/SpriteBase.hpp>
#include <Components/Persistent/PlayerRenderOffset.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/Cadaver.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/Curse.hpp>
#include <Components/Player/Doglegs.hpp>
#include <Components/Player/ExtraLife.hpp>
#include <Components/Player/FootstepType.hpp>
#include <Components/Player/GraveTrail.hpp>
#include <Components/Player/HeartBeat.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/LastGraveyardPosition.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Player/MovementDelta.hpp>
#include <Components/Player/SelectedInventorySlot.hpp>
#include <Components/Player/TorchRadius.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/SpawnArea.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/OfferingAction.hpp>
#include <Components/Stats/PlayerStats.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Factory/AltarFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/PlantFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Factory/ToxicityFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/System/Vector2.hpp>
#include <entt/entity/fwd.hpp>
#include <spdlog/spdlog.h>

namespace Game::Factory::Player
{

void create_player( entt::registry &reg, const PathFinding::SpatialHashGridSharedPtr &reserved_sm )
{
  SPDLOG_DEBUG( "Creating player entity" );
  auto entity = reg.create();

  // start position must be pixel coordinates within the screen resolution (kDisplaySize),
  // but also grid aligned (kMapGridSize) to avoid collision detection errors.
  // So we must recalc start position to the nearest grid position here
  auto start_pos = Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( reg );
  start_pos = Utils::snap_to_grid( start_pos );
  Cmp::Position player_pos( start_pos, Constants::kGridSizePxF );
  reg.emplace_or_replace<Cmp::Position>( entity, player_pos );
  reg.emplace_or_replace<Cmp::Player::Character>( entity );
  if ( reserved_sm ) reserved_sm->insert( entity, player_pos );
  reg.emplace_or_replace<Cmp::PlayerStats>( entity, Utils::Player::make_starting_stats( reg ) );

  reg.emplace_or_replace<Cmp::Direction>( entity, sf::Vector2f{ 0, 0 } );
  reg.emplace_or_replace<Cmp::LastDirection>( entity, sf::Vector2f{ 0, 0 } );
  reg.emplace_or_replace<Cmp::Player::MovementDelta>( entity );
  reg.emplace_or_replace<Cmp::Player::Footstep>( entity, Cmp::Player::Footstep::Type::GRASS );
  reg.emplace_or_replace<Cmp::Toxicity::Toxidrome>( entity, Factory::Toxicity::ToxidromeBuilder<>{}.build() );

  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( entity, Cmp::AnimData::Config{ 
        .sprite_type = "sprite.player.walk.south", 
        .current_frame = 1,  // middle cell of three frames: standing still
        .enabled = true
  });
  // clang-format on

  std::vector<Sys::SpriteKey> list = {
      "sprite.crypt.loot.cadaver.skull",    "sprite.crypt.loot.cadaver.torso",   "sprite.crypt.loot.cadaver.leftarm",
      "sprite.crypt.loot.cadaver.rightarm", "sprite.crypt.loot.cadaver.leftleg", "sprite.crypt.loot.cadaver.rightleg",
  };
  reg.emplace_or_replace<Cmp::Player::Cadaver>( entity, list );
  reg.emplace_or_replace<Cmp::Player::Wealth>( entity, 0 );
  reg.emplace_or_replace<Cmp::Player::Mortality>( entity, Cmp::Player::Mortality::State::ALIVE );
  reg.emplace_or_replace<Cmp::Player::Curse>( entity, false );
  reg.emplace_or_replace<Cmp::Player::LevelDepth>( entity, 1 );
  reg.emplace_or_replace<Cmp::TorchRadius>( entity, 32.f );

  reg.emplace_or_replace<Cmp::ZOrderValue>( entity, start_pos.y ); // z-order based on y-position
  reg.emplace_or_replace<Cmp::AbsoluteAlpha>( entity, 255 );       // fully opaque
  reg.emplace_or_replace<Cmp::AbsoluteRotation>( entity, 0 );
  reg.emplace_or_replace<Cmp::AbsoluteRenderOffset>(
      entity, sf::Vector2f{ 0.f, Sys::PersistSystem::get<Cmp::Persist::PlayerRenderOffset>( reg ).get_value() } );
  reg.emplace_or_replace<Cmp::Player::HeartBeat>( entity );
  reg.emplace_or_replace<Cmp::Player::Doglegs>( entity );
  reg.emplace_or_replace<Cmp::Grimoire>( entity );
  reg.emplace_or_replace<Cmp::Player::GraveTrail>( entity );
  reg.emplace_or_replace<Cmp::Player::SelectedInventorySlot>( entity );
  ensure_inventory_slots( reg );
}

entt::entity add_spawn_area( entt::registry &reg, entt::entity entity, float zorder )
{
  // We need to reserve these positions for the player start area, dont add NpcNoPathFinding.
  // We want NPCs to pathfind player within spawn. We block NPCs from entering spawn directly in NpcSystem::update_pathfinding.
  reg.emplace_or_replace<Cmp::SpawnArea>( entity, false );
  auto idx = Sys::SpriteStore::instance().get( "sprite.graveyard.playerspawn" ).get_random_texture_index();
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( entity, Cmp::AnimData::Config{ 
        .sprite_type = "sprite.graveyard.playerspawn", 
        .frame_index_offset = idx,
        .enabled = true
  });
  // clang-format on

  reg.emplace_or_replace<Cmp::ZOrderValue>( entity, zorder );
  return entity;
}

void create_player_death_anim( entt::registry &reg, Cmp::Position player_pos_cmp, const Sprites::SpriteSheet &sprite )
{
  auto player_blood_splat_entity = reg.create();
  sf::Vector2f offset;
  if ( ( sprite.sprite_size().x == Constants::kGridSizePxF.x ) and ( sprite.sprite_size().y == Constants::kGridSizePxF.y ) )
  {
    offset = sf::Vector2f{ 0, 0 };
  }
  else { offset = sprite.sprite_size() / 2.f; }
  reg.emplace_or_replace<Cmp::Position>( player_blood_splat_entity, player_pos_cmp.position - offset, player_pos_cmp.size );
  reg.emplace_or_replace<Cmp::DeathPosition>( player_blood_splat_entity, player_pos_cmp.position - offset, player_pos_cmp.size );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( player_blood_splat_entity, Cmp::AnimData::Config{ 
        .sprite_type = sprite.type(), 
        .enabled = true,
        .anim_type = Cmp::AnimType::ONESHOTHOLD
  });
  // clang-format on
  reg.emplace_or_replace<Cmp::ZOrderValue>( player_blood_splat_entity, player_pos_cmp.position.y * 3 ); // always infront
}

entt::entity create_empty_inventory_slot( entt::registry &reg, uint8_t idx )
{
  auto slot_entt = reg.create();
  reg.emplace<Cmp::PlayerInventorySlot>( slot_entt, idx, std::nullopt );
  return slot_entt;
}

void clear_inventory_slot( entt::registry &reg, entt::entity slot_entt )
{
  auto *slot_cmp = reg.try_get<Cmp::PlayerInventorySlot>( slot_entt );
  if ( not slot_cmp ) return;
  slot_cmp->m_item.reset();
  // every per-item component a slot can carry. Add any new per-item component here, so an emptied slot never keeps
  // stale item state. The entity itself is kept so the slot never moves.
  reg.remove<Cmp::Inventory::WearLevel, Cmp::Inventory::StingDamage, Cmp::UUID, Cmp::AnimData, Cmp::Inventory::DowsingTarget,
             Cmp::Npc::SummonOffering, Cmp::Grave::CurseTablet, Cmp::ZOrderValue>( slot_entt );
}

void ensure_inventory_slots( entt::registry &reg )
{
  const auto count = Utils::Player::get_inventory_slot_count( reg );
  auto slot_view = reg.view<Cmp::PlayerInventorySlot>();

  std::array<bool, Cmp::kMaxInventorySlots> present{};
  std::vector<entt::entity> surplus;
  for ( auto [slot_entt, slot_cmp] : slot_view.each() )
  {
    if ( slot_cmp.m_slot_idx >= count or present.at( slot_cmp.m_slot_idx ) ) { surplus.push_back( slot_entt ); }
    else { present.at( slot_cmp.m_slot_idx ) = true; }
  }

  // the slot count can only be changed from the settings menu, between runs, so surplus slots should be empty
  for ( auto slot_entt : surplus )
  {
    const auto &slot_cmp = reg.get<Cmp::PlayerInventorySlot>( slot_entt );
    if ( not slot_cmp.empty() )
    {
      SPDLOG_WARN( "Removing inventory slot {} holding {}: slot count is now {}", slot_cmp.m_slot_idx, slot_cmp.m_item->item_type.str(), count );
    }
    reg.destroy( slot_entt );
  }

  for ( uint8_t idx = 0; idx < count; ++idx )
  {
    if ( not present.at( idx ) ) { create_empty_inventory_slot( reg, idx ); }
  }
}

void destroy_inventory( entt::registry &reg, const Sys::ItemKey &type )
{
  // prefer the selected slot, so using the held item never empties a different slot holding the same item
  const auto selected_entt = Utils::Player::get_selected_slot_entt( reg );
  if ( selected_entt != entt::null )
  {
    const auto &selected_cmp = reg.get<Cmp::PlayerInventorySlot>( selected_entt );
    if ( not selected_cmp.empty() and selected_cmp.m_item->item_type == type )
    {
      clear_inventory_slot( reg, selected_entt );
      return;
    }
  }
  for ( uint8_t idx = 0; idx < Utils::Player::get_inventory_slot_count( reg ); ++idx )
  {
    const auto slot_entt = Utils::Player::get_inventory_slot_entt( reg, idx );
    if ( slot_entt == entt::null ) continue;
    const auto &slot_cmp = reg.get<Cmp::PlayerInventorySlot>( slot_entt );
    if ( slot_cmp.empty() or slot_cmp.m_item->item_type != type ) continue;
    clear_inventory_slot( reg, slot_entt );
    return;
  }
}

Cmp::Position add_player_last_graveyard_pos( entt::registry &reg, const Cmp::Position &last_known_pos, [[maybe_unused]] sf::Vector2f offset )
{
  SPDLOG_INFO( "Player will re-enter grave yard at {},{}", last_known_pos.position.x, last_known_pos.position.y );
  auto player_entt = Utils::Player::get_entity( reg );
  reg.emplace_or_replace<Cmp::Player::LastGraveyardPosition>( player_entt, last_known_pos.position, last_known_pos.size );
  return last_known_pos;
}

void remove_player_last_graveyard_pos( entt::registry &reg )
{
  auto player_entt = Utils::Player::get_entity( reg );
  reg.remove<Cmp::Player::LastGraveyardPosition>( player_entt );
}

void remove_player_extra_life( entt::registry &reg )
{
  auto player_entt = Utils::Player::get_entity( reg );
  reg.remove<Cmp::Player::ExtraLife>( player_entt );
  reg.remove<Cmp::FlashUIExtraLife>( player_entt );

  // deactivate the altar that granted the extra life, if it is in this scene
  Factory::Altar::deactivate_altars( reg );
}

} // namespace Game::Factory::Player
