#ifndef SRC_SCENECONTROL_REGISTRYTRANSFER_HPP__
#define SRC_SCENECONTROL_REGISTRYTRANSFER_HPP__

#include <SceneControl/SceneStack.hpp>
#include <entt/entity/fwd.hpp>
#include <memory>

namespace Game::Scene
{

//! @brief Enumeration for registry copy modes
enum class RegCopyMode {
  //! @brief Copy no components
  NONE,
  //! @brief Copy only player components
  PLAYER_ONLY,
  //! @brief Copy all non-blacklisted entities and components
  ALL
};

//! @brief Copies selected entities/components between scene ECS registries during scene
//!        transitions (e.g. carrying the player's state from one scene's registry into the next).
class RegistryTransfer
{
public:
  //! @brief Type alias for a unique pointer to an entt::registry
  using RegCopy = std::unique_ptr<entt::registry>;

  //! @brief Legacy/unused variant of copy_reg(); declared but not currently implemented.
  //! @param scene The scene to copy the registry from
  //! @param copy_mode The mode to use for copying
  //! @return RegCopy A unique pointer to the copied registry
  RegCopy copy_reg_old( IScene &scene, RegCopyMode copy_mode = RegCopyMode::NONE );

  //! @brief Create a deep copy of selected entities from a scene's ECS registry into a new
  //!        standalone registry, to preserve state (e.g. the player) across scene transitions.
  //! @param scene The scene to copy the registry from
  //! @param copy_mode The mode to use for copying
  //! @return RegCopy A unique pointer to the copied registry
  RegCopy copy_reg( IScene &scene, RegCopyMode copy_mode = RegCopyMode::NONE );

  //! @brief Sync two registries for player entity components only.
  //! @details Use this to transfer player owned components between scenes.
  //!          Note the component must be registered via `init_missing_cmp_storages()` in order for this to work
  //! @param source_registry Source registry
  //! @param target_registry Target registry
  void xfer_player_entt( entt::registry &source_registry, entt::registry &target_registry );

  //! @brief Sync two registries for a specific component only.
  //! @details Use this to transfer components between scenes.
  //!          Note the component must be registered via `init_missing_cmp_storages()` in order for this to work
  //! @tparam COMPONENT
  //! @param source_registry Copy from
  //! @param target_registry Copy to
  template <typename COMPONENT>
  void xfer_component_entt( entt::registry &source_registry, entt::registry &target_registry )
  {
    auto source_inventory_view = source_registry.view<COMPONENT>();
    if ( source_inventory_view.empty() )
    {
      SPDLOG_WARN( "Player inventory is empty in source regsitry. Removing inventory in target registry" );
      // Note, this kills all slots in target registry of the next scene, but since we only have one slot this doesnt matter
      // If we add more slots, you would compare slot ids here.
      auto target_inventory_view = target_registry.view<COMPONENT>();
      for ( auto inventory_entity : target_inventory_view )
      {
        target_registry.destroy( inventory_entity );
      }
      return;
    }

    // auto source_entity = inventory_view.front();
    for ( auto source_entity : source_inventory_view )
    {

      // Check if player entity already exists in target registry
      auto target_inventory_view = target_registry.view<COMPONENT>();
      entt::entity target_entity;

      if ( target_inventory_view.empty() )
      {
        // No player exists, create new one
        target_entity = target_registry.create();
        SPDLOG_DEBUG( "Created new inventory entity (#{}) in target registry", static_cast<uint32_t>( target_entity ) );
      }
      else
      {
        // Player exists, use existing entity
        target_entity = target_inventory_view.front();
        SPDLOG_DEBUG( "Using existing inventory entity (#{}) in target registry", static_cast<uint32_t>( target_entity ) );
      }

      // Ensure all known player component storages exist in target registry
      init_missing_cmp_storages( target_registry );

      // Create a copy of an entity component by component (from entt wiki)
      std::vector<std::string> transferred_cmps;
      std::vector<std::string> removed_cmps;
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
          else { SPDLOG_WARN( "No storage found in target reg for cmp: {}", source_storage.type().name() ); }
        }
      }
      pretty_print( "Removed", removed_cmps );
      pretty_print( "Transferred", transferred_cmps );
    }
  }

private:
  //! @brief Allow transfer on allow list components. Ensure all known player component storages exist in the given registry
  //! @param registry
  void init_missing_cmp_storages( entt::registry &registry );

  //! @brief Print python style list of component type names
  //! @param components Vector of component type names
  void pretty_print( const std::string &prefix, const std::vector<std::string> &components );
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_REGISTRYTRANSFER_HPP__
