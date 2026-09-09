#include <Components/AbsoluteAlpha.hpp>
#include <Components/AbsoluteRotation.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/Crypt/Lever.hpp>
#include <Components/Crypt/ObjectiveMultiBlock.hpp>
#include <Components/Crypt/PassageBlock.hpp>
#include <Components/Crypt/RoomClosed.hpp>
#include <Components/Crypt/RoomEnd.hpp>
#include <Components/Crypt/RoomOpen.hpp>
#include <Components/Crypt/RoomStart.hpp>
#include <Components/Direction.hpp>
#include <Components/FootStepAlpha.hpp>
#include <Components/FootStepTimer.hpp>
#include <Components/Inventory/Explosive.hpp>
#include <Components/Inventory/Grimoire.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/ScryingBall.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LastDirection.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Player/BlastRadius.hpp>
#include <Components/Player/CadaverCount.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/Curse.hpp>
#include <Components/Player/FootstepType.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/LastGraveyardPosition.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Player/MovementDelta.hpp>
#include <Components/Player/RuinLocation.hpp>
#include <Components/Player/TorchRadius.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/Ruin/ObjectiveType.hpp>
#include <Components/Stats/PlayerStats.hpp>

#include <Components/UUID.hpp>
#include <Components/VoidPosition.hpp>
#include <Components/ZOrderValue.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SceneControl/RegistryTransfer.hpp>

#include <spdlog/spdlog.h>

namespace Game::Scene
{

//! @brief Creates a deep copy of selected entities from a scene's ECS registry into a new standalone registry. The function supports different copy
//! modes to control what gets transferred during scene transitions — either just the player entity's state, all non-blacklisted entities, or nothing
//! at all.
//! @details
//! 1. Early exit check — Returns nullptr if copy_mode is NONE
//! 2. Initialize target registry — Creates a new entt::registry and pre-registers known component storages via init_missing_cmp_storages()
//! 3. Copy player entity (for PLAYER_ONLY or ALL modes):
//!   Find the entity with Character component
//!   Create a new entity in the target registry
//!   Iterate all component storages and copy each component the player has
//!   Skip components without registered storage (e.g., LerpPosition)
//! 4. Copy other entities (for ALL mode only):
//!   Iterate all entities in source registry
//!   Skip the player (already copied)
//!   Skip entities with blacklisted components (map/crypt-specific data)
//!   For each valid entity, create a new entity and copy all its components
//! @param scene
//! @param copy_mode
//! @return RegistryTransfer::RegCopy
RegistryTransfer::RegCopy RegistryTransfer::copy_reg( IScene &scene, Scene::RegCopyMode copy_mode )
{
  if ( copy_mode == RegCopyMode::NONE ) { return nullptr; }

  auto registry_copy = std::make_unique<entt::registry>();
  auto &source_registry = scene.registry();

  init_missing_cmp_storages( *registry_copy );

  [[maybe_unused]] int skipped_cmp = 0;
  std::vector<std::string> copied_cmp;
  std::vector<std::string> no_storage_cmps;

  // Copy player entity (for PLAYER_ONLY and ALL modes)
  if ( copy_mode == RegCopyMode::PLAYER_ONLY || copy_mode == RegCopyMode::ALL )
  {
    auto player_view = source_registry.view<Cmp::Player::Character>();
    if ( !player_view.empty() )
    {
      auto player_entity = player_view.front();
      auto new_entity = registry_copy->create();

      // Copy all components from player entity
      for ( auto &&curr : source_registry.storage() )
      {
        if ( auto &source_storage = curr.second; source_storage.contains( player_entity ) )
        {
          auto type_hash = curr.first;
          if ( auto *target_storage = registry_copy->storage( type_hash ) )
          {
            target_storage->push( new_entity, source_storage.value( player_entity ) );
            SPDLOG_DEBUG( "Copied player component: {}", source_storage.type().name() );
            copied_cmp.emplace_back( source_storage.type().name() );
          }
          else { SPDLOG_WARN( "No storage found in target registry for component: {}", source_storage.type().name() ); }
        }
      }
      SPDLOG_DEBUG( "Player entity copied with {} components", copied_cmp.size() );
    }
    else { SPDLOG_WARN( "No player entity found in source registry to copy" ); }
  }

  // Copy other entities (for ALL mode only)
  if ( copy_mode == RegCopyMode::ALL )
  {
    auto reserved_sm = scene.get_reserved_sm();
    for ( auto entity : source_registry.storage<entt::entity>() )
    {
      // Skip player entity (already copied above)
      if ( source_registry.any_of<Cmp::Player::Character>( entity ) ) { continue; }

      // Skip transfer on deny list components
      if ( source_registry.any_of<Cmp::Obstacle, Cmp::Armable, Cmp::Npc::NoPathFinding, Cmp::FootStepTimer, Cmp::FootStepAlpha, Cmp::Crypt::RoomOpen,
                                  Cmp::Crypt::RoomClosed, Cmp::Crypt::RoomStart, Cmp::Crypt::RoomEnd, Cmp::Crypt::PassageBlock, Cmp::Crypt::Lever,
                                  Cmp::Crypt::ObjectiveMultiBlock, Cmp::VoidPosition>( entity ) )
      {
        skipped_cmp++;
        continue;
      }

      // Skip transfer of positions reserved from algorithmic changes (structural world entities)
      if ( reserved_sm )
      {
        if ( auto *pos_cmp = source_registry.try_get<Cmp::Position>( entity ); pos_cmp && not reserved_sm->at( *pos_cmp ).empty() )
        {
          skipped_cmp++;
          continue;
        }
      }

      auto new_entity = registry_copy->create();

      for ( auto &&curr : source_registry.storage() )
      {
        if ( auto &source_storage = curr.second; source_storage.contains( entity ) )
        {
          auto type_hash = curr.first;
          if ( auto *target_storage = registry_copy->storage( type_hash ) )
          {
            target_storage->push( new_entity, source_storage.value( entity ) );
            SPDLOG_DEBUG( "Copied component: {}", source_storage.type().name() );
            copied_cmp.emplace_back( source_storage.type().name() );
          }
          else { no_storage_cmps.emplace_back( source_storage.type().name() ); }
        }
      }
    }
  }
  pretty_print( "Copied", copied_cmp );
  pretty_print( "No Storage Found", no_storage_cmps );

  return registry_copy;
}

void RegistryTransfer::xfer_player_entt( entt::registry &source_registry, entt::registry &target_registry )
{
  auto player_view = source_registry.view<Cmp::Player::Character>();
  if ( player_view.empty() )
  {
    SPDLOG_WARN( "No player entity found to transfer" );
    return;
  }

  auto source_entity = player_view.front();

  // Check if player entity already exists in target registry
  auto target_player_view = target_registry.view<Cmp::Player::Character>();
  entt::entity target_entity;

  if ( target_player_view.empty() )
  {
    // No player exists, create new one
    target_entity = target_registry.create();
    SPDLOG_DEBUG( "Created new player entity (#{}) in target registry", static_cast<uint32_t>( target_entity ) );
  }
  else
  {
    // Player exists, use existing entity
    target_entity = target_player_view.front();
    SPDLOG_DEBUG( "Using existing player entity (#{}) in target registry", static_cast<uint32_t>( target_entity ) );
  }

  // Ensure all known player component storages exist in target registry
  init_missing_cmp_storages( target_registry );

  // Create a copy of an entity component by component (from entt wiki)
  std::vector<std::string> transferred_cmps;
  std::vector<std::string> removed_cmps;
  std::vector<std::string> no_storage_cmps;
  for ( auto &&curr : source_registry.storage() )
  {
    if ( auto &source_storage = curr.second; source_storage.contains( source_entity ) )
    {
      SPDLOG_DEBUG( "Transferring component: {}", source_storage.type().name() );

      auto type_hash = curr.first;

      if ( auto *target_storage = target_registry.storage( type_hash ) )
      {
        if ( target_storage->contains( target_entity ) )
        {
          target_storage->erase( target_entity );
          SPDLOG_DEBUG( "Removed existing component: {}", source_storage.type().name() );
          removed_cmps.emplace_back( source_storage.type().name() );
        }
        target_storage->push( target_entity, source_storage.value( source_entity ) );
        transferred_cmps.emplace_back( source_storage.type().name() );

        SPDLOG_DEBUG( "Successfully transferred component: {}", source_storage.type().name() );
      }
      else { no_storage_cmps.emplace_back( source_storage.type().name() ); }
    }
  }
  pretty_print( "Removed", removed_cmps );
  pretty_print( "Transferred", transferred_cmps );
  pretty_print( "No Storage Found", no_storage_cmps );
}

void RegistryTransfer::init_missing_cmp_storages( entt::registry &registry )
{
  // Force storage creation by accessing storage for each known component type
  registry.storage<Cmp::AbsoluteAlpha>();
  registry.storage<Cmp::AbsoluteRotation>();
  registry.storage<Cmp::Direction>();
  registry.storage<Cmp::LastDirection>();
  registry.storage<Cmp::Player::MovementDelta>();
  registry.storage<Cmp::Position>();
  registry.storage<Cmp::Player::Character>();
  registry.storage<Cmp::Player::LevelDepth>();
  registry.storage<Cmp::PlayerStats>();
  registry.storage<Cmp::Player::Wealth>();
  registry.storage<Cmp::Player::Curse>();
  registry.storage<Cmp::Player::Footstep>();
  registry.storage<Cmp::Player::BlastRadius>();
  registry.storage<Cmp::Player::Mortality>();
  registry.storage<Cmp::Player::CadaverCount>();
  registry.storage<Cmp::AnimData>();
  registry.storage<Cmp::PlayerInventorySlot>();
  registry.storage<Cmp::TorchRadius>();
  registry.storage<Cmp::WorldItem>();
  registry.storage<Cmp::Inventory::WearLevel>();
  registry.storage<Cmp::ZOrderValue>();
  registry.storage<Cmp::SeeingStone>();
  registry.storage<Cmp::Explosive>();
  registry.storage<Cmp::Player::LastGraveyardPosition>();
  registry.storage<Cmp::Player::RuinLocation>();
  registry.storage<Cmp::Ruin::ObjectiveType>();
  registry.storage<Cmp::UUID>();
  registry.storage<Cmp::Grimoire>();
  // Add other player-related components as needed
}

void RegistryTransfer::pretty_print( const std::string &prefix, const std::vector<std::string> &components )
{
  if ( components.empty() ) return;

  std::stringstream ss;
  ss << "[ ";
  for ( const auto &comp_name : components )
  {
    // get class from namespace string after last "::"
    auto last_colon = comp_name.find_last_of( "::" );
    if ( last_colon != std::string::npos ) { ss << comp_name.substr( last_colon + 1 ) << " "; }
    else { ss << comp_name << " "; }
  }
  ss << "]";
  SPDLOG_INFO( "{} [{}]: {}", prefix, components.size(), ss.str() );
}

} // namespace Game::Scene