#include <algorithm>
#include <cmath>

#include <Components/AbsoluteAlpha.hpp>
#include <Components/AbsoluteRenderOffset.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/AnimData.hpp>
#include <Components/Crypt/RoomLavaPitCell.hpp>
#include <Components/DamagePopup.hpp>
#include <Components/Direction.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Health.hpp>
#include <Components/Grave/CurseTablet.hpp>
#include <Components/Inventory/DowsingTarget.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/StackCount.hpp>
#include <Components/Inventory/StingDamage.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LastDirection.hpp>
#include <Components/LerpPosition.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/SummonOffering.hpp>
#include <Components/Npc/Wisp.hpp>
#include <Components/Particle/SpriteOwner.hpp>
#include <Components/Persistent/DiggingCooldownThreshold.hpp>
#include <Components/Persistent/InventorySlotCount.hpp>
#include <Components/Persistent/InventoryStackSize.hpp>
#include <Components/Persistent/StowedTorchLit.hpp>
#include <Components/Persistent/BradycardiaMovementSpeed.hpp>
#include <Components/Persistent/MaxLevelDepth.hpp>
#include <Components/Persistent/ShowDamageText.hpp>
#include <Components/Persistent/PlayerMovementSpeed.hpp>
#include <Components/Persistent/TachycardiaMovementSpeed.hpp>
#include <Components/Persistent/StartingDespair.hpp>
#include <Components/Persistent/StartingFear.hpp>
#include <Components/Persistent/StartingHealth.hpp>
#include <Components/Persistent/StartingInfamy.hpp>
#include <Components/Persistent/StartingLuck.hpp>
#include <Components/Plant/BurningTimer.hpp>
#include <Components/Player/Cadaver.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/ConsumeTimer.hpp>
#include <Components/Player/Curse.hpp>
#include <Components/Player/DiggingTimer.hpp>
#include <Components/Player/Doglegs.hpp>
#include <Components/Player/ExtraLife.hpp>
#include <Components/Player/Flying.hpp>
#include <Components/Player/FootstepType.hpp>
#include <Components/Player/HeartBeat.hpp>
#include <Components/Player/Illuminated.hpp>
#include <Components/Player/LastGraveyardPosition.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Player/RuinLocation.hpp>
#include <Components/Player/SelectedInventorySlot.hpp>
#include <Components/Player/SpeedPenalty.hpp>
#include <Components/Player/TookDamage.hpp>
#include <Components/Player/TorchRadius.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <Components/SceneSettings/CurrentScene.hpp>
#include <Components/SpawnArea.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/CollisionAction.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ForageAction.hpp>
#include <Components/Stats/OfferingAction.hpp>
#include <Components/Stats/ProjectileAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/GrimoireItemUpdateEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <stdexcept>
#include <type_traits>

namespace Game::Utils::Player
{

entt::entity get_entity( entt::registry &reg )
{
  auto player_view = reg.view<Cmp::Player::Character, const Cmp::Position>();
  if ( player_view.front() == entt::null ) throw std::runtime_error( "Player entity could not be found" );
  return player_view.front();
}

//! @brief Get the player position object
//! @note To prevent navmesh de-sync, you must modify the player position using `UpdatePlayerPositionEvent`.
//! @throws runtime_error if player has no Cmp::Position
//! @param reg
//! @return Cmp::Position&
const Cmp::Position &get_position( entt::registry &reg )
{
  auto player_view = reg.view<Cmp::Player::Character, const Cmp::Position>();
  for ( auto [entt, player_cmp, pos_cmp] : player_view.each() )
  {
    return pos_cmp;
  }
  throw std::runtime_error( "Player entt has no component: Cmp::Position" );
}

//! @brief Get the player direction object
//! @throws runtime_error if player has no Cmp::Direction
//! @param reg
//! @return Cmp::Direction&
Cmp::Direction &get_direction( entt::registry &reg )
{
  auto player_view = reg.view<Cmp::Player::Character, Cmp::Direction>();
  for ( auto [entt, player_cmp, dir_cmp] : player_view.each() )
  {
    return dir_cmp;
  }
  throw std::runtime_error( "Player entt has no component: Cmp::Direction" );
}

//! @brief Get the player last direction object
//! @throws runtime_error if player has no Cmp::LastDirection
//! @param reg
//! @return Cmp::LastDirection&
Cmp::LastDirection &get_last_direction( entt::registry &reg )
{
  auto player_view = reg.view<Cmp::Player::Character, Cmp::LastDirection>();
  for ( auto [entt, player_cmp, dir_cmp] : player_view.each() )
  {
    return dir_cmp;
  }
  throw std::runtime_error( "Player entt has no component: Cmp::LastDirection" );
}

//! @brief Get the player sprite anim object
//! @throws runtime_error if player has no Cmp::SpriteAnimation
//! @param reg
//! @return Cmp::SpriteAnimation&
Cmp::AnimData &get_sprite_anim( entt::registry &reg )
{
  auto player_view = reg.view<Cmp::Player::Character, Cmp::AnimData>();
  for ( auto [entt, player_cmp, anim_cmp] : player_view.each() )
  {
    return anim_cmp;
  }
  throw std::runtime_error( "Player entt has no component: Cmp::SpriteAnimation" );
}

int get_ruin_location( entt::registry &reg )
{
  auto *player_location = reg.try_get<Cmp::Player::RuinLocation>( get_entity( reg ) );
  if ( player_location ) { return static_cast<int>( player_location->m_floor ); }
  return static_cast<int>( Cmp::Player::RuinLocation::Floor::NONE );
}

//! @brief Get the player last graveyard position object. Return may be nullptr if non-existent.
//! @param reg
//! @return Cmp::Player::LastGraveyardPosition*
Cmp::Player::LastGraveyardPosition *get_last_graveyard_pos( entt::registry &reg )
{
  auto player_entt = get_entity( reg );
  return reg.try_get<Cmp::Player::LastGraveyardPosition>( player_entt );
}

Cmp::Player::LevelDepth &get_level_depth( entt::registry &reg )
{
  auto *level_depth = reg.try_get<Cmp::Player::LevelDepth>( get_entity( reg ) );
  if ( not level_depth ) throw std::runtime_error( "Player entt has no component: Cmp::Player::LevelDepth" );
  return *level_depth;
}

bool is_final_level_depth( entt::registry &reg )
{
  const int max_depth = std::max( 1, Sys::PersistSystem::get<Cmp::Persist::MaxLevelDepth>( reg ).get_value() );
  return get_level_depth( reg ).get_count() >= static_cast<unsigned int>( max_depth );
}

Cmp::Player::Wealth &get_wealth( entt::registry &reg )
{
  auto *wealth = reg.try_get<Cmp::Player::Wealth>( get_entity( reg ) );
  if ( not wealth ) throw std::runtime_error( "Player entt has no component: Cmp::Player::Wealth" );
  return *wealth;
}

Cmp::Player::Mortality &get_mortality( entt::registry &reg )
{
  auto *mortality = reg.try_get<Cmp::Player::Mortality>( get_entity( reg ) );
  if ( not mortality ) throw std::runtime_error( "Player entt has no component: Cmp::Player::Mortality" );
  return *mortality;
}

Cmp::ZOrderValue &get_zorder( entt::registry &reg )
{
  auto *zorder_cmp = reg.try_get<Cmp::ZOrderValue>( get_entity( reg ) );
  if ( not zorder_cmp ) throw std::runtime_error( "Player entt has no component: Cmp::ZOrderValue" );

  auto player_view = reg.view<Cmp::Player::Character, Cmp::ZOrderValue>();
  return player_view.get<Cmp::ZOrderValue>( get_entity( reg ) );
}

Cmp::AbsoluteAlpha &get_alpha( entt::registry &reg )
{
  auto *alpha_cmp = reg.try_get<Cmp::AbsoluteAlpha>( get_entity( reg ) );
  if ( not alpha_cmp ) throw std::runtime_error( "Player entt has no component: Cmp::AbsoluteAlpha" );

  auto player_view = reg.view<Cmp::Player::Character, Cmp::AbsoluteAlpha>();
  return player_view.get<Cmp::AbsoluteAlpha>( get_entity( reg ) );
}

Cmp::Player::Curse &get_curse( entt::registry &reg )
{
  auto *curse = reg.try_get<Cmp::Player::Curse>( get_entity( reg ) );
  if ( not curse ) throw std::runtime_error( "Player entt has no component: Cmp::Player::Curse" );
  SPDLOG_DEBUG( "Cmp::Player::Curse == {}", curse->active );
  return *curse;
}

void reset_curse( entt::registry &reg )
{
  auto *curse = reg.try_get<Cmp::Player::Curse>( get_entity( reg ) );
  if ( not curse ) throw std::runtime_error( "Player entt has no component: Cmp::Player::Curse" );
  curse->active = false;
  curse->shader_alpha.reset();
  SPDLOG_DEBUG( "Cmp::Player::Curse == {}", curse->active );
}

float get_movement_speed( entt::registry &reg )
{
  const float base_speed = Sys::PersistSystem::get<Cmp::Persist::PlayerMovementSpeed>( reg ).get_value();
  const auto &toxidrome = get_stats( reg ).toxidrome();

  // Tachycardia/Bradycardia are mutually exclusive, normalise within -1.0 to 1.0 (see PlayerSystem::update_heartbeat)
  const float tachycardia = static_cast<float>( toxidrome.at<Cmp::Toxicity::Tachycardia>().value_or( 0 ) );
  const float bradycardia = static_cast<float>( toxidrome.at<Cmp::Toxicity::Bradycardia>().value_or( 0 ) );
  const float severity = std::clamp( ( tachycardia - bradycardia ) / 100.f, -1.f, 1.f );

  if ( severity >= 0.f )
  {
    const float tachy_speed = Sys::PersistSystem::get<Cmp::Persist::TachycardiaMovementSpeed>( reg ).get_value();
    return std::lerp( base_speed, tachy_speed, severity );
  }
  const float brady_speed = Sys::PersistSystem::get<Cmp::Persist::BradycardiaMovementSpeed>( reg ).get_value();
  return std::lerp( base_speed, brady_speed, -severity );
}

float get_speed_penalty( entt::registry &reg )
{
  auto *penalty_cmp = reg.try_get<Cmp::Player::SpeedPenalty>( get_entity( reg ) );
  float penalty = penalty_cmp ? penalty_cmp->m_penalty : 1.f;
  // a flying player passes over burrs and grass on the ground
  if ( not is_flying( reg ) )
  {
    penalty *= Utils::get_burr_speed_multiplier( reg, get_entity( reg ), get_position( reg ) );
    penalty *= Utils::get_grass_speed_multiplier( reg, get_position( reg ) );
  }
  return penalty;
}

void remove_lerp_cmp( entt::registry &reg )
{
  // Clear any ongoing lerp from the previous scene to prevent invalid player re-positioning
  auto player_entt = get_entity( reg );
  if ( reg.any_of<Cmp::LerpPosition>( player_entt ) )
  {
    reg.remove<Cmp::LerpPosition>( player_entt );
    SPDLOG_DEBUG( "Cleared LerpPosition component to prevent position interpolation" );
  }
}

uint8_t get_inventory_slot_count( entt::registry &reg )
{
  const int count = Sys::PersistSystem::get<Cmp::Persist::InventorySlotCount>( reg ).get_value();
  return static_cast<uint8_t>( std::clamp( count, 1, static_cast<int>( Cmp::kMaxInventorySlots ) ) );
}

uint8_t get_selected_slot_idx( entt::registry &reg )
{
  const auto count = get_inventory_slot_count( reg );
  for ( auto [player_entt, selected_cmp] : reg.view<Cmp::Player::SelectedInventorySlot>().each() )
  {
    return std::min<uint8_t>( selected_cmp.m_idx, count - 1 );
  }
  return 0;
}

entt::entity get_inventory_slot_entt( entt::registry &reg, uint8_t idx )
{
  for ( auto [slot_entt, slot_cmp] : reg.view<Cmp::PlayerInventorySlot>().each() )
  {
    if ( slot_cmp.m_slot_idx == idx ) return slot_entt;
  }
  return entt::null;
}

std::optional<uint8_t> get_first_empty_slot_idx( entt::registry &reg )
{
  const auto count = get_inventory_slot_count( reg );
  for ( uint8_t idx = 0; idx < count; ++idx )
  {
    const auto slot_entt = get_inventory_slot_entt( reg, idx );
    if ( slot_entt == entt::null or reg.get<Cmp::PlayerInventorySlot>( slot_entt ).empty() ) return idx;
  }
  return std::nullopt;
}

int get_stack_count( entt::registry &reg, entt::entity entt )
{
  const auto *stack_cmp = reg.try_get<Cmp::Inventory::StackCount>( entt );
  return stack_cmp ? stack_cmp->m_count : 1;
}

bool can_stack( entt::registry &reg, entt::entity slot_entt, entt::entity world_item_entt )
{
  if ( not reg.valid( slot_entt ) or not reg.valid( world_item_entt ) ) return false;
  const auto *slot_cmp = reg.try_get<Cmp::PlayerInventorySlot>( slot_entt );
  const auto *world_item_cmp = reg.try_get<Cmp::WorldItem>( world_item_entt );
  if ( not slot_cmp or slot_cmp->empty() or not world_item_cmp ) return false;
  if ( not world_item_cmp->is_stackable() or slot_cmp->m_item->item_type != world_item_cmp->item_type ) return false;
  if ( reg.all_of<Cmp::PlantMultiBlock>( world_item_entt ) ) return false;

  // a stack is one set of item state, so items carrying their own state never merge. Every world item has a UUID
  // (see Sys::ItemSystem::create_world_item()), but it is only state when a particle (e.g. a torch's flame) is bound to it
  auto has_bound_particle = [&reg]( entt::entity entt )
  {
    const auto *uuid_cmp = reg.try_get<Cmp::UUID>( entt );
    if ( not uuid_cmp ) return false;
    for ( auto [ps_entt, ps_owner, ps_uuid_cmp] : reg.view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
    {
      if ( ps_uuid_cmp == *uuid_cmp ) return true;
    }
    return false;
  };
  auto has_instance_state = [&reg, &has_bound_particle]( entt::entity entt )
  {
    return reg.any_of<Cmp::Inventory::WearLevel, Cmp::Inventory::DowsingTarget, Cmp::Npc::SummonOffering, Cmp::Grave::CurseTablet>( entt ) or
           has_bound_particle( entt );
  };
  if ( has_instance_state( slot_entt ) or has_instance_state( world_item_entt ) ) return false;

  const int max_stack = Sys::PersistSystem::get<Cmp::Persist::InventoryStackSize>( reg ).get_value();
  return get_stack_count( reg, slot_entt ) < max_stack;
}

entt::entity find_stackable_slot( entt::registry &reg, entt::entity world_item_entt )
{
  if ( const auto selected_entt = get_selected_slot_entt( reg ); can_stack( reg, selected_entt, world_item_entt ) ) return selected_entt;
  const auto count = get_inventory_slot_count( reg );
  for ( uint8_t idx = 0; idx < count; ++idx )
  {
    const auto slot_entt = get_inventory_slot_entt( reg, idx );
    if ( slot_entt != entt::null and can_stack( reg, slot_entt, world_item_entt ) ) return slot_entt;
  }
  return entt::null;
}

entt::entity get_selected_slot_entt( entt::registry &reg ) { return get_inventory_slot_entt( reg, get_selected_slot_idx( reg ) ); }

std::tuple<entt::entity, Sys::ItemKey, Sys::SpriteKey> get_inventory( entt::registry &reg )
{
  const auto slot_entt = get_selected_slot_entt( reg );
  if ( slot_entt == entt::null ) return { entt::null, {}, {} };
  const auto &slot_cmp = reg.get<Cmp::PlayerInventorySlot>( slot_entt );
  if ( slot_cmp.empty() ) return { entt::null, {}, {} };
  return { slot_entt, slot_cmp.m_item->item_type, slot_cmp.m_item->sprite_type };
}

entt::entity get_inventory_entt( entt::registry &reg )
{
  const auto slot_entt = get_selected_slot_entt( reg );
  if ( slot_entt == entt::null or reg.get<Cmp::PlayerInventorySlot>( slot_entt ).empty() ) return entt::null;
  return slot_entt;
}

float get_inventory_wear_level( entt::registry &reg )
{
  const auto slot_entt = get_inventory_entt( reg );
  if ( slot_entt == entt::null ) return -1;
  if ( const auto *wear_level = reg.try_get<Cmp::Inventory::WearLevel>( slot_entt ) ) return wear_level->m_level;
  SPDLOG_DEBUG( "Player Inventory slot has no appropriate WearLevel component" );
  return -1;
}

bool is_torch_slot_lit( entt::registry &reg, entt::entity slot_entt )
{
  const auto *slot_cmp = reg.try_get<Cmp::PlayerInventorySlot>( slot_entt );
  if ( not slot_cmp or slot_cmp->empty() or slot_cmp->m_item->item_type != "item.torch" ) return false;
  if ( slot_cmp->m_slot_idx == get_selected_slot_idx( reg ) ) return true;
  return Sys::PersistSystem::get<Cmp::Persist::StowedTorchLit>( reg ).get_value();
}

bool has_lit_torch( entt::registry &reg )
{
  for ( auto slot_entt : reg.view<Cmp::PlayerInventorySlot>() )
  {
    if ( is_torch_slot_lit( reg, slot_entt ) ) return true;
  }
  return false;
}

bool is_elderflute_active( entt::registry &reg )
{
  if ( not Utils::is_current_scene( reg, Cmp::SceneSettings::SceneId::GRAVEYARD ) ) return false;
  auto [_, inventory_type, _] = get_inventory( reg );
  return inventory_type == "item.elderflute" and get_inventory_wear_level( reg ) > 0;
}

bool is_flying( entt::registry &reg )
{
  for ( [[maybe_unused]] auto entt : reg.view<Cmp::Player::Character, Cmp::Player::Flying>() )
  {
    return true;
  }
  return false;
}

bool is_landing( entt::registry &reg )
{
  const auto *flying_cmp = reg.try_get<Cmp::Player::Flying>( get_entity( reg ) );
  return flying_cmp and flying_cmp->landing;
}

bool is_flying_elevated( entt::registry &reg )
{
  const auto *flying_cmp = reg.try_get<Cmp::Player::Flying>( get_entity( reg ) );
  return flying_cmp and flying_cmp->elevated;
}

bool is_inventory_item_active( entt::registry &reg )
{
  auto [_, inventory_type, _] = get_inventory( reg );
  if ( inventory_type == "item.elderflute" ) return is_elderflute_active( reg );
  if ( inventory_type == "item.witchesbroom" ) return is_flying( reg );
  return false;
}

bool is_in_spawn( entt::registry &reg, const Cmp::Position &player_pos_cmp )
{
  bool result = false;
  for ( auto [spawn_entt, spawn_cmp, spawn_pos_cmp] : reg.view<Cmp::SpawnArea, Cmp::Position>().each() )
  {
    if ( spawn_pos_cmp.findIntersection( Cmp::RectBounds::scaled( player_pos_cmp, 0.9f ).getBounds() ) ) result = true;
  }
  return result;
}

bool is_illuminated( entt::registry &reg ) { return reg.all_of<Cmp::Player::Illuminated>( get_entity( reg ) ); }

bool is_player_near( entt::registry &reg, const sf::FloatRect &bounds, float player_hitbox_scale )
{
  for ( auto [pc_entt, pc_cmp, pc_pos_cmp] : reg.view<Cmp::Player::Character, const Cmp::Position>().each() )
  {
    auto player_hitbox = Cmp::RectBounds::scaled( pc_pos_cmp.position, Constants::kGridSizePxF, player_hitbox_scale );
    if ( player_hitbox.findIntersection( bounds ) ) return true;
  }
  return false;
}

bool is_digging_on_cooldown( entt::registry &reg )
{
  auto digging_cooldown_amount = Sys::PersistSystem::get<Cmp::Persist::DiggingCooldownThreshold>( reg ).get_value();
  auto *player_dig_cooldown = reg.try_get<Cmp::Player::DiggingTimer>( get_entity( reg ) );
  return ( player_dig_cooldown != nullptr ) and * player_dig_cooldown < sf::seconds( digging_cooldown_amount );
}

//! @brief The grid cell one tile in front of the player, based on their last facing direction
Cmp::Position get_projected_position( entt::registry &reg )
{
  auto player_pos = get_position( reg );
  auto player_last_direction = get_last_direction( reg );
  return { { player_pos.getCenter().x + ( player_last_direction.x * Constants::kGridSizePxF.x ),
             player_pos.getCenter().y + ( player_last_direction.y * Constants::kGridSizePxF.y ) },
           { 1.f, 1.f } };
}

Cmp::Player::Cadaver &get_cadaver( entt::registry &reg )
{

  auto *cadaver = reg.try_get<Cmp::Player::Cadaver>( get_entity( reg ) );
  if ( not cadaver ) throw std::runtime_error( "Player entt has no component: Cmp::Player::CadaverCount" );
  SPDLOG_DEBUG( "Cmp::Player::CadaverCount == {}", curse->active );
  return *cadaver;
}

Cmp::TorchRadius &get_torch_radius( entt::registry &reg )
{

  auto *torch_radius = reg.try_get<Cmp::TorchRadius>( get_entity( reg ) );
  if ( not torch_radius ) throw std::runtime_error( "Player entt has no component: Cmp::TorchRadius" );
  SPDLOG_DEBUG( "Cmp::TorchRadius == {}", torch_radius->value );
  return *torch_radius;
}

Cmp::PlayerStats &get_stats( entt::registry &reg )
{
  auto player_stats_view = reg.view<Cmp::PlayerStats>();
  for ( auto [entity, cmp] : player_stats_view.each() )
  {
    if ( reg.all_of<Cmp::Player::Character>( entity ) ) { return cmp; }
  }
  throw std::runtime_error( "Player entt has no component: Cmp::PlayerStats" );
}

Cmp::PlayerStats make_starting_stats( entt::registry &reg )
{
  return Cmp::PlayerStats( Cmp::Stats::Health{ Sys::PersistSystem::get<Cmp::Persist::StartingHealth>( reg ).get_value() },
                           Cmp::Stats::Fear{ Sys::PersistSystem::get<Cmp::Persist::StartingFear>( reg ).get_value() },
                           Cmp::Stats::Despair{ Sys::PersistSystem::get<Cmp::Persist::StartingDespair>( reg ).get_value() },
                           Cmp::Stats::Infamy{ Sys::PersistSystem::get<Cmp::Persist::StartingInfamy>( reg ).get_value() },
                           Cmp::Stats::Luck{ Sys::PersistSystem::get<Cmp::Persist::StartingLuck>( reg ).get_value() } );
}

void reset_stats_to_starting( entt::registry &reg )
{
  auto &stats = get_stats( reg );
  const auto target = make_starting_stats( reg );
  // PlayerStats only takes deltas, so move each stat by the difference to its starting value
  stats.apply( Cmp::BaseAction( Cmp::Stats::Health{ target.health() - stats.health() }, Cmp::Stats::Fear{ target.fear() - stats.fear() },
                                Cmp::Stats::Despair{ target.despair() - stats.despair() },
                                Cmp::Stats::Infamy{ target.infamy() - stats.infamy() }, Cmp::Stats::Luck{ target.luck() - stats.luck() }, {} ) );
}

Cmp::Player::Footstep &get_footstep_type( entt::registry &reg )
{
  auto player_entt = get_entity( reg );
  auto *footstep_cmp = reg.try_get<Cmp::Player::Footstep>( player_entt );
  if ( not footstep_cmp ) throw std::runtime_error( "Player has no Cmp::Player::Footstep component." );
  return *footstep_cmp;
}

sf::Time &get_global_bomb_flash_timer( entt::registry &reg )
{
  auto player_view = reg.view<Cmp::Player::Character>();
  for ( auto [entt, player_cmp] : player_view.each() )
  {
    return player_cmp.m_global_bomb_flash_timer;
  }
  throw std::runtime_error( "Player entt has no component: Cmp::Player::Character" );
}

bool player_has_extra_life( entt::registry &reg )
{
  auto player_view = reg.view<Cmp::Player::ExtraLife>();
  return not player_view.empty();
}

//! @brief Record the death state implied by a damaging item action, for use if the player's health reaches zero.
template <typename ActionT>
void record_item_damage_cause( entt::registry &reg, const Cmp::BaseAction &action )
{
  if ( action.health() >= 0 ) return;
  auto &mortality = get_mortality( reg );
  if constexpr ( std::is_same_v<ActionT, Cmp::ConsumeAction> ) { mortality.last_damage_cause = Cmp::Player::Mortality::State::POISONED; }
  else if constexpr ( std::is_same_v<ActionT, Cmp::BurnAction> ) { mortality.last_damage_cause = Cmp::Player::Mortality::State::IGNITED; }
  else { mortality.last_damage_cause = Cmp::Player::Mortality::State::SKEWERED; }
}

//! @brief A spawn_action or carry_action only damages the player while its item has a Cmp::Inventory::StingDamage.
template <typename ActionT>
void remove_damage_if_no_sting( entt::registry &reg, entt::entity item_entt, Cmp::BaseAction &action )
{
  if constexpr ( std::is_same_v<ActionT, Cmp::SpawnAction> or std::is_same_v<ActionT, Cmp::CarryAction> )
  {
    if ( not reg.all_of<Cmp::Inventory::StingDamage>( item_entt ) ) action.remove_damage();
  }
}

template <typename ActionT>
Cmp::BaseAction apply_action_from_world_item( entt::registry &reg, entt::entity world_item_entt, const std::source_location &loc )
{
  auto *plant_item = reg.try_get<Cmp::WorldItem>( world_item_entt );
  if ( not plant_item )
  {
    throw std::runtime_error( std::string( loc.file_name() ) + "Unable to get Cmp::WorldItem from entt " +
                              std::to_string( static_cast<uint32_t>( world_item_entt ) ) + ". Entity is " +
                              std::string( reg.valid( world_item_entt ) ? "valid" : "invalid" ) );
  }
  auto action = plant_item->at<ActionT>().action;
  remove_damage_if_no_sting<ActionT>( reg, world_item_entt, action );
  record_item_damage_cause<ActionT>( reg, action );
  Utils::Player::get_stats( reg ).apply( action );
  Sys::BaseSystem::get_systems_event_queue().trigger( Events::GrimoireItemUpdateEvent( plant_item->item_type, ActionT::kKey, action ) );
  return action;
}

// Explicit instantiations for every Cmp::BaseAction subclass (see src/Components/Stats) - keeps the
// template body out of Player.hpp while still allowing all known action kinds to be applied.
template Cmp::BaseAction apply_action_from_world_item<Cmp::BurnAction>( entt::registry &, entt::entity, const std::source_location & );
template Cmp::BaseAction apply_action_from_world_item<Cmp::CarryAction>( entt::registry &, entt::entity, const std::source_location & );
template Cmp::BaseAction apply_action_from_world_item<Cmp::CollisionAction>( entt::registry &, entt::entity, const std::source_location & );
template Cmp::BaseAction apply_action_from_world_item<Cmp::ConsumeAction>( entt::registry &, entt::entity, const std::source_location & );
template Cmp::BaseAction apply_action_from_world_item<Cmp::DestroyAction>( entt::registry &, entt::entity, const std::source_location & );
template Cmp::BaseAction apply_action_from_world_item<Cmp::ForageAction>( entt::registry &, entt::entity, const std::source_location & );
template Cmp::BaseAction apply_action_from_world_item<Cmp::ProjectileAction>( entt::registry &, entt::entity, const std::source_location & );
template Cmp::BaseAction apply_action_from_world_item<Cmp::ProximityAction>( entt::registry &, entt::entity, const std::source_location & );
template Cmp::BaseAction apply_action_from_world_item<Cmp::OfferingAction>( entt::registry &, entt::entity, const std::source_location & );
template Cmp::BaseAction apply_action_from_world_item<Cmp::SpawnAction>( entt::registry &, entt::entity, const std::source_location & );

template <typename ActionT>
Cmp::BaseAction apply_action_from_inventory_item( entt::registry &reg )
{
  Cmp::BaseAction applied_action( {}, {}, {}, {}, {}, {} );
  const auto inventory_entt = get_inventory_entt( reg );
  if ( inventory_entt == entt::null )
  {
    SPDLOG_WARN( "Player has no inventory. Unable to apply action modifiers" );
    return applied_action;
  }
  const auto &inventory_cmp = reg.get<Cmp::PlayerInventorySlot>( inventory_entt );
  auto &player_stats = Utils::Player::get_stats( reg );
  auto action = inventory_cmp.m_item->at<ActionT>().action;
  remove_damage_if_no_sting<ActionT>( reg, inventory_entt, action );
  if ( action.health() < 0 ) reg.emplace_or_replace<Cmp::Player::TookDamage>( Utils::Player::get_entity( reg ) );
  record_item_damage_cause<ActionT>( reg, action );
  player_stats.apply( action );
  Sys::BaseSystem::get_systems_event_queue().trigger(
      Events::GrimoireItemUpdateEvent( inventory_cmp.m_item->item_type, ActionT::kKey, action ) );
  applied_action = action;
  return applied_action;
}

// Explicit instantiations for every Cmp::BaseAction subclass (see src/Components/Stats) - keeps the
// template body out of Player.hpp while still allowing all known action kinds to be applied.
template Cmp::BaseAction apply_action_from_inventory_item<Cmp::BurnAction>( entt::registry & );
template Cmp::BaseAction apply_action_from_inventory_item<Cmp::CarryAction>( entt::registry & );
template Cmp::BaseAction apply_action_from_inventory_item<Cmp::CollisionAction>( entt::registry & );
template Cmp::BaseAction apply_action_from_inventory_item<Cmp::ConsumeAction>( entt::registry & );
template Cmp::BaseAction apply_action_from_inventory_item<Cmp::DestroyAction>( entt::registry & );
template Cmp::BaseAction apply_action_from_inventory_item<Cmp::ForageAction>( entt::registry & );
template Cmp::BaseAction apply_action_from_inventory_item<Cmp::ProjectileAction>( entt::registry & );
template Cmp::BaseAction apply_action_from_inventory_item<Cmp::ProximityAction>( entt::registry & );
template Cmp::BaseAction apply_action_from_inventory_item<Cmp::OfferingAction>( entt::registry & );
template Cmp::BaseAction apply_action_from_inventory_item<Cmp::SpawnAction>( entt::registry & );

template <typename ActionT>
Cmp::BaseAction apply_action_from_item_store( entt::registry &reg, const Sys::ItemKey &item_type )
{
  const auto &item = Sys::ItemStore::instance().get( item_type );
  auto action = item.at<ActionT>().action;
  record_item_damage_cause<ActionT>( reg, action );
  Utils::Player::get_stats( reg ).apply( action );
  Sys::BaseSystem::get_systems_event_queue().trigger( Events::GrimoireItemUpdateEvent( item_type, ActionT::kKey, action ) );
  return action;
}

// Explicit instantiations for every Cmp::BaseAction subclass (see src/Components/Stats) - keeps the
// template body out of Player.hpp while still allowing all known action kinds to be applied.
template Cmp::BaseAction apply_action_from_item_store<Cmp::BurnAction>( entt::registry &, const Sys::ItemKey & );
template Cmp::BaseAction apply_action_from_item_store<Cmp::CarryAction>( entt::registry &, const Sys::ItemKey & );
template Cmp::BaseAction apply_action_from_item_store<Cmp::CollisionAction>( entt::registry &, const Sys::ItemKey & );
template Cmp::BaseAction apply_action_from_item_store<Cmp::ConsumeAction>( entt::registry &, const Sys::ItemKey & );
template Cmp::BaseAction apply_action_from_item_store<Cmp::DestroyAction>( entt::registry &, const Sys::ItemKey & );
template Cmp::BaseAction apply_action_from_item_store<Cmp::ForageAction>( entt::registry &, const Sys::ItemKey & );
template Cmp::BaseAction apply_action_from_item_store<Cmp::ProjectileAction>( entt::registry &, const Sys::ItemKey & );
template Cmp::BaseAction apply_action_from_item_store<Cmp::ProximityAction>( entt::registry &, const Sys::ItemKey & );
template Cmp::BaseAction apply_action_from_item_store<Cmp::OfferingAction>( entt::registry &, const Sys::ItemKey & );
template Cmp::BaseAction apply_action_from_item_store<Cmp::SpawnAction>( entt::registry &, const Sys::ItemKey & );

template <typename ActionT>
Cmp::BaseAction get_action_from_item_store( const Sys::ItemKey &item_type )
{
  const auto &item = Sys::ItemStore::instance().get( item_type );
  return item.at<ActionT>().action;
}

// Explicit instantiations for every Cmp::BaseAction subclass (see src/Components/Stats) - keeps the
// template body out of Player.hpp while still allowing all known action kinds to be applied.
template Cmp::BaseAction get_action_from_item_store<Cmp::BurnAction>( const Sys::ItemKey & );
template Cmp::BaseAction get_action_from_item_store<Cmp::CarryAction>( const Sys::ItemKey & );
template Cmp::BaseAction get_action_from_item_store<Cmp::CollisionAction>( const Sys::ItemKey & );
template Cmp::BaseAction get_action_from_item_store<Cmp::ConsumeAction>( const Sys::ItemKey & );
template Cmp::BaseAction get_action_from_item_store<Cmp::DestroyAction>( const Sys::ItemKey & );
template Cmp::BaseAction get_action_from_item_store<Cmp::ForageAction>( const Sys::ItemKey & );
template Cmp::BaseAction get_action_from_item_store<Cmp::ProjectileAction>( const Sys::ItemKey & );
template Cmp::BaseAction get_action_from_item_store<Cmp::ProximityAction>( const Sys::ItemKey & );
template Cmp::BaseAction get_action_from_item_store<Cmp::OfferingAction>( const Sys::ItemKey & );
template Cmp::BaseAction get_action_from_item_store<Cmp::SpawnAction>( const Sys::ItemKey & );

Cmp::Player::HeartBeat &get_heartbeat( entt::registry &reg )
{
  auto player_entt = get_entity( reg );
  auto *heartbeat_cmp = reg.try_get<Cmp::Player::HeartBeat>( player_entt );
  if ( not heartbeat_cmp ) throw std::runtime_error( "Player has no Cmp::Player::HeartBeat component." );
  return *heartbeat_cmp;
}

Cmp::Player::ConsumeTimer *is_player_eating( entt::registry &reg )
{
  auto player_entt = Utils::Player::get_entity( reg );
  return reg.try_get<Cmp::Player::ConsumeTimer>( player_entt );
}

Cmp::Player::Doglegs &get_doglegs( entt::registry &reg )
{
  auto player_entt = get_entity( reg );
  // scene-local cache that is not transferred between scenes, so create it on demand
  return reg.get_or_emplace<Cmp::Player::Doglegs>( player_entt );
}

bool apply_weapon_hit( entt::registry &reg, entt::entity target_entt, const Sys::ItemKey &weapon )
{
  auto *health_cmp = reg.try_get<Cmp::Health>( target_entt );
  if ( not health_cmp ) return false;

  // a plant's weapon damage is in its items.json entry, an NPC's in its npc.json entry. The popup rises from the top-centre
  Cmp::WeaponDamage damage_from;
  Cmp::WeaponDamage damage_to;
  sf::Vector2f popup_pos;
  if ( const auto *plant_mb_cmp = reg.try_get<Cmp::PlantMultiBlock>( target_entt ) )
  {
    const auto &plant_item = reg.get<Cmp::WorldItem>( target_entt );
    damage_from = plant_item.damage_from_weapon();
    damage_to = plant_item.damage_to_weapon();
    popup_pos = { plant_mb_cmp->position.x + ( plant_mb_cmp->size.x / 2.f ), plant_mb_cmp->position.y };
  }
  else if ( const auto *npc_cmp = reg.try_get<Cmp::Npc::NPC>( target_entt ) )
  {
    damage_from = npc_cmp->damage_from_weapon;
    damage_to = npc_cmp->damage_to_weapon;
    const auto &pos_cmp = reg.get<Cmp::Position>( target_entt );
    popup_pos = { pos_cmp.position.x + ( pos_cmp.size.x / 2.f ), pos_cmp.position.y };
    if ( const auto *render_offset_cmp = reg.try_get<Cmp::AbsoluteRenderOffset>( target_entt ) ) popup_pos += render_offset_cmp->getOffset();
  }
  else { return false; }

  auto &event_queue = Sys::BaseSystem::get_systems_event_queue();
  auto [held_entt, held_type, _] = get_inventory( reg );
  if ( const int wear = damage_to.get( weapon ); wear > 0 and held_type == weapon )
  {
    event_queue.trigger( Events::UpdateDmgEvent( held_entt, static_cast<float>( wear ) ) );
  }

  const auto damage = static_cast<float>( damage_from.get( weapon ) );
  if ( damage <= 0.f ) return false;
  // the popup shows the health actually lost, so overkill on a weak target reads as its remaining health
  const float health_lost = std::min( damage, health_cmp->m_level );
  event_queue.trigger( Events::UpdateDmgEvent( target_entt, damage ) );

  if ( Sys::PersistSystem::get<Cmp::Persist::ShowDamageText>( reg ).get_value() and health_lost > 0.f )
  {
    reg.emplace<Cmp::DamagePopup>( reg.create(), Cmp::DamagePopup{ .world_pos = popup_pos, .amount = health_lost } );
  }
  // re-fetch: the event handlers may have touched the storage
  const auto *after_cmp = reg.try_get<Cmp::Health>( target_entt );
  return after_cmp and after_cmp->m_level <= 0.f;
}

} // namespace Game::Utils::Player