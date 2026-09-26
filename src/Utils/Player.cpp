#include <Components/AbsoluteAlpha.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/AnimData.hpp>
#include <Components/Crypt/RoomLavaPitCell.hpp>
#include <Components/Direction.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LastDirection.hpp>
#include <Components/LerpPosition.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/Wisp.hpp>
#include <Components/Particle/SpriteOwner.hpp>
#include <Components/Plant/BurningTimeAccumulator.hpp>
#include <Components/Player/BlastRadius.hpp>
#include <Components/Player/CadaverCount.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/Curse.hpp>
#include <Components/Player/ExtraLife.hpp>
#include <Components/Player/FootstepType.hpp>
#include <Components/Player/HeartBeat.hpp>
#include <Components/Player/Illuminated.hpp>
#include <Components/Player/LastGraveyardPosition.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Player/RuinLocation.hpp>
#include <Components/Player/SpeedPenalty.hpp>
#include <Components/Player/TorchRadius.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <Components/SpawnArea.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/CollisionAction.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ProjectileAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/SacrificeAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Sprites/SpriteMetaType.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>

#include <stdexcept>

namespace Game::Utils::Player
{

entt::entity get_entity( entt::registry &reg )
{
  auto player_view = reg.view<Cmp::Player::Character, Cmp::Position>();
  if ( player_view.front() == entt::null ) throw std::runtime_error( "Player entity could not be found" );
  return player_view.front();
}

//! @brief Get the player position object
//! @throws runtime_error if player has no Cmp::Position
//! @param reg
//! @return Cmp::Position&
Cmp::Position &get_position( entt::registry &reg )
{
  auto player_view = reg.view<Cmp::Player::Character, Cmp::Position>();
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

Cmp::Player::Wealth &get_wealth( entt::registry &reg )
{
  auto *wealth = reg.try_get<Cmp::Player::Wealth>( get_entity( reg ) );
  if ( not wealth ) throw std::runtime_error( "Player entt has no component: Cmp::Player::Wealth" );
  return *wealth;
}

Cmp::Player::BlastRadius &get_blast_radius( entt::registry &reg )
{
  auto *blast_radius = reg.try_get<Cmp::Player::BlastRadius>( get_entity( reg ) );
  if ( not blast_radius ) throw std::runtime_error( "Player entt has no component: Cmp::Player::BlastRadius" );
  return *blast_radius;
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

float get_speed_penalty( entt::registry &reg )
{
  auto *penalty_cmp = reg.try_get<Cmp::Player::SpeedPenalty>( get_entity( reg ) );
  if ( penalty_cmp ) { return penalty_cmp->m_penalty; }
  return 1.f;
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

std::tuple<entt::entity, std::string, Sprites::SpriteMetaType> get_inventory( entt::registry &reg )
{
  auto inv_view = reg.view<Cmp::PlayerInventorySlot>();
  Sprites::SpriteMetaType found_sprite_type;
  std::string found_inv_type;
  entt::entity found_entt = entt::null;
  // this assumes there is only one slot in the inventory, so warn if there is a bug somewhere
  if ( inv_view.size() > 1 ) throw std::runtime_error( "Found multiple slots in single slot inventory" );
  for ( auto [inv_entt, inv_cmp] : inv_view.each() )
  {
    found_inv_type = inv_cmp.m_item.item_type;
    found_sprite_type = inv_cmp.m_item.sprite_type;
    found_entt = inv_entt;
  }
  return { found_entt, found_inv_type, found_sprite_type };
}

float get_inventory_wear_level( entt::registry &reg )
{
  auto inventory_wear_view = reg.view<Cmp::PlayerInventorySlot, Cmp::Inventory::WearLevel>();
  for ( auto [inventory_entity, inventory_slot, wear_level] : inventory_wear_view.each() )
  {
    return wear_level.m_level;
  }
  SPDLOG_DEBUG( "Player Inventory slot has no appropriate WearLevel component" );
  return -1;
}

void reduce_inventory_wear_level( entt::registry &reg, float amount )
{
  auto inventory_wear_view = reg.view<Cmp::PlayerInventorySlot, Cmp::Inventory::WearLevel>();
  for ( auto [inventory_entity, inventory_slot, wear_level] : inventory_wear_view.each() )
  {
    wear_level.m_level -= amount;
    return;
  }
  SPDLOG_DEBUG( "Player Inventory slot has no appropriate WearLevel component" );
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

//! @brief True if the player's hitbox (scaled 1.5x the grid size) intersects the given bounds
bool is_player_near( entt::registry &reg, const sf::FloatRect &bounds )
{
  for ( auto [pc_entt, pc_cmp, pc_pos_cmp] : reg.view<Cmp::Player::Character, Cmp::Position>().each() )
  {
    auto player_hitbox = Cmp::RectBounds::scaled( pc_pos_cmp.position, Constants::kGridSizePxF, 1.5f );
    if ( player_hitbox.findIntersection( bounds ) ) return true;
  }
  return false;
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

Cmp::Player::CadaverCount &get_cadaver_count( entt::registry &reg )
{

  auto *curse = reg.try_get<Cmp::Player::CadaverCount>( get_entity( reg ) );
  if ( not curse ) throw std::runtime_error( "Player entt has no component: Cmp::Player::CadaverCount" );
  SPDLOG_DEBUG( "Cmp::Player::CadaverCount == {}", curse->active );
  return *curse;
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

Cmp::Player::Footstep &get_footstep_type( entt::registry &reg )
{
  auto player_entt = get_entity( reg );
  auto *footstep_cmp = reg.try_get<Cmp::Player::Footstep>( player_entt );
  if ( not footstep_cmp ) throw std::runtime_error( "Player has no Cmp::Player::Footstep component." );
  return *footstep_cmp;
}

sf::Clock &get_global_bomb_flash_clk( entt::registry &reg )
{
  auto player_view = reg.view<Cmp::Player::Character>();
  for ( auto [entt, player_cmp] : player_view.each() )
  {
    return player_cmp.m_global_bomb_flash_clk;
  }
  throw std::runtime_error( "Player entt has no component: Cmp::Player::Character" );
}

bool player_has_extra_life( entt::registry &reg )
{
  auto player_view = reg.view<Cmp::Player::ExtraLife>();
  return not player_view.empty();
}

template <typename ActionT>
void apply_action_from_world_item( entt::registry &reg, entt::entity world_item_entt, const std::source_location &loc )
{
  auto *plant_item = reg.try_get<Cmp::WorldItem>( world_item_entt );
  if ( not plant_item )
  {
    throw std::runtime_error( std::string( loc.file_name() ) + "Unable to get Cmp::WorldItem from entt " +
                              std::to_string( static_cast<uint32_t>( world_item_entt ) ) + ". Entity is " +
                              std::string( reg.valid( world_item_entt ) ? "valid" : "invalid" ) );
  }
  Utils::Player::get_stats( reg ).apply( plant_item->at<ActionT>().action );
}

// Explicit instantiations for every Cmp::BaseAction subclass (see src/Components/Stats) - keeps the
// template body out of Player.hpp while still allowing all known action kinds to be applied.
template void apply_action_from_world_item<Cmp::BurnAction>( entt::registry &, entt::entity, const std::source_location & );
template void apply_action_from_world_item<Cmp::CarryAction>( entt::registry &, entt::entity, const std::source_location & );
template void apply_action_from_world_item<Cmp::CollisionAction>( entt::registry &, entt::entity, const std::source_location & );
template void apply_action_from_world_item<Cmp::ConsumeAction>( entt::registry &, entt::entity, const std::source_location & );
template void apply_action_from_world_item<Cmp::DestroyAction>( entt::registry &, entt::entity, const std::source_location & );
template void apply_action_from_world_item<Cmp::ProjectileAction>( entt::registry &, entt::entity, const std::source_location & );
template void apply_action_from_world_item<Cmp::ProximityAction>( entt::registry &, entt::entity, const std::source_location & );
template void apply_action_from_world_item<Cmp::SacrificeAction>( entt::registry &, entt::entity, const std::source_location & );
template void apply_action_from_world_item<Cmp::SpawnAction>( entt::registry &, entt::entity, const std::source_location & );

template <typename ActionT>
void apply_action_from_inventory_item( entt::registry &reg )
{
  auto inventory_view = reg.view<Cmp::PlayerInventorySlot>();
  if ( inventory_view->empty() )
  {
    SPDLOG_WARN( "Player has no inventory. Unable to apply action modifiers" );
    return;
  }
  for ( auto [inventory_entt, inventory_cmp] : inventory_view.each() )
  {
    auto &player_stats = Utils::Player::get_stats( reg );
    player_stats.apply( inventory_cmp.m_item.at<ActionT>().action );
  }
}

// Explicit instantiations for every Cmp::BaseAction subclass (see src/Components/Stats) - keeps the
// template body out of Player.hpp while still allowing all known action kinds to be applied.
template void apply_action_from_inventory_item<Cmp::BurnAction>( entt::registry & );
template void apply_action_from_inventory_item<Cmp::CarryAction>( entt::registry & );
template void apply_action_from_inventory_item<Cmp::CollisionAction>( entt::registry & );
template void apply_action_from_inventory_item<Cmp::ConsumeAction>( entt::registry & );
template void apply_action_from_inventory_item<Cmp::DestroyAction>( entt::registry & );
template void apply_action_from_inventory_item<Cmp::ProjectileAction>( entt::registry & );
template void apply_action_from_inventory_item<Cmp::ProximityAction>( entt::registry & );
template void apply_action_from_inventory_item<Cmp::SacrificeAction>( entt::registry & );
template void apply_action_from_inventory_item<Cmp::SpawnAction>( entt::registry & );

template <typename ActionT>
void apply_action_from_item_store( entt::registry &reg, const std::string &item_type )
{
  auto item = Sys::ItemStore::instance().get_item( item_type );
  Utils::Player::get_stats( reg ).apply( item.at<ActionT>().action );
}

// Explicit instantiations for every Cmp::BaseAction subclass (see src/Components/Stats) - keeps the
// template body out of Player.hpp while still allowing all known action kinds to be applied.
template void apply_action_from_item_store<Cmp::BurnAction>( entt::registry &, const std::string & );
template void apply_action_from_item_store<Cmp::CarryAction>( entt::registry &, const std::string & );
template void apply_action_from_item_store<Cmp::CollisionAction>( entt::registry &, const std::string & );
template void apply_action_from_item_store<Cmp::ConsumeAction>( entt::registry &, const std::string & );
template void apply_action_from_item_store<Cmp::DestroyAction>( entt::registry &, const std::string & );
template void apply_action_from_item_store<Cmp::ProjectileAction>( entt::registry &, const std::string & );
template void apply_action_from_item_store<Cmp::ProximityAction>( entt::registry &, const std::string & );
template void apply_action_from_item_store<Cmp::SacrificeAction>( entt::registry &, const std::string & );
template void apply_action_from_item_store<Cmp::SpawnAction>( entt::registry &, const std::string & );

template <typename ActionT>
Cmp::BaseAction get_action_from_item_store( const std::string &item_type )
{
  auto item = Sys::ItemStore::instance().get_item( item_type );
  return item.at<ActionT>().action;
}

// Explicit instantiations for every Cmp::BaseAction subclass (see src/Components/Stats) - keeps the
// template body out of Player.hpp while still allowing all known action kinds to be applied.
template Cmp::BaseAction get_action_from_item_store<Cmp::BurnAction>( const std::string & );
template Cmp::BaseAction get_action_from_item_store<Cmp::CarryAction>( const std::string & );
template Cmp::BaseAction get_action_from_item_store<Cmp::CollisionAction>( const std::string & );
template Cmp::BaseAction get_action_from_item_store<Cmp::ConsumeAction>( const std::string & );
template Cmp::BaseAction get_action_from_item_store<Cmp::DestroyAction>( const std::string & );
template Cmp::BaseAction get_action_from_item_store<Cmp::ProjectileAction>( const std::string & );
template Cmp::BaseAction get_action_from_item_store<Cmp::ProximityAction>( const std::string & );
template Cmp::BaseAction get_action_from_item_store<Cmp::SacrificeAction>( const std::string & );
template Cmp::BaseAction get_action_from_item_store<Cmp::SpawnAction>( const std::string & );

template <typename ActionT>
void apply_action_from_npc_store( entt::registry &reg, const std::string &npc_type )
{
  auto npc = Sys::NpcStore::instance().get_item( npc_type );
  Utils::Player::get_stats( reg ).apply( npc.at<ActionT>().action );
}

// Explicit instantiations for every Cmp::BaseAction subclass (see src/Components/Stats) - keeps the
// template body out of Player.hpp while still allowing all known action kinds to be applied.
template void apply_action_from_npc_store<Cmp::BurnAction>( entt::registry &, const std::string & );
template void apply_action_from_npc_store<Cmp::CarryAction>( entt::registry &, const std::string & );
template void apply_action_from_npc_store<Cmp::CollisionAction>( entt::registry &, const std::string & );
template void apply_action_from_npc_store<Cmp::ConsumeAction>( entt::registry &, const std::string & );
template void apply_action_from_npc_store<Cmp::DestroyAction>( entt::registry &, const std::string & );
template void apply_action_from_npc_store<Cmp::ProjectileAction>( entt::registry &, const std::string & );
template void apply_action_from_npc_store<Cmp::ProximityAction>( entt::registry &, const std::string & );
template void apply_action_from_npc_store<Cmp::SacrificeAction>( entt::registry &, const std::string & );
template void apply_action_from_npc_store<Cmp::SpawnAction>( entt::registry &, const std::string & );

Cmp::Player::HeartBeat &get_heartbeat( entt::registry &reg )
{
  auto player_entt = get_entity( reg );
  auto *heartbeat_cmp = reg.try_get<Cmp::Player::HeartBeat>( player_entt );
  if ( not heartbeat_cmp ) throw std::runtime_error( "Player has no Cmp::Player::HeartBeat component." );
  return *heartbeat_cmp;
}

} // namespace Game::Utils::Player