#include <Audio/SoundBank.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Persistent/RuinMaxCobwebs.hpp>
#include <Components/Persistent/RuinProcGenBirthThreshold.hpp>
#include <Components/Persistent/RuinProcGenInitChance.hpp>
#include <Components/Persistent/RuinProcGenMaxIterations.hpp>
#include <Components/Persistent/RuinProcGenSurvivalThreshold.hpp>
#include <Components/Player/Curse.hpp>
#include <Components/Player/RuinLocation.hpp>
#include <Components/Ruin/FloorAccess.hpp>
#include <Components/Ruin/ObjectiveType.hpp>
#include <Components/Ruin/StairsLowerMultiBlock.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/SceneSettings/CurrentScene.hpp>
#include <Components/SceneSettings/Footsteps.hpp>
#include <Components/SceneSettings/Shaders.hpp>
#include <Components/SceneSettings/ShowDebugStats.hpp>
#include <Components/SceneSettings/ShowNavmesh.hpp>
#include <Components/SceneSettings/ShowPathFinding.hpp>

#include <Factory/NpcFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PathfindingFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Factory/RuinFactory.hpp>
#include <Factory/ShaderFactory.hpp>
#include <Factory/WallFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SceneControl/SceneData.hpp>
#include <SceneControl/Scenes/RuinSceneLowerFloor.hpp>
#include <Systems/ActionSystem.hpp>
#include <Systems/AnimSystem.hpp>
#include <Systems/CryptSystem.hpp>
#include <Systems/FootstepSystem.hpp>
#include <Systems/HealingSpringSystem.hpp>
#include <Systems/ItemSystem.hpp>
#include <Systems/LootSystem.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/PlayerSystem.hpp>
#include <Systems/ProcGen/CellAutomataSystem.hpp>
#include <Systems/ProcGen/DLASystem.hpp>
#include <Systems/ProcGen/LevelGenerator.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/Render/RenderOverlaySystem.hpp>
#include <Systems/RuinSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Systems/Threats/NpcSystem.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Audio/Sound.hpp>

namespace Game::Scene
{

void RuinSceneLowerFloor::on_init()
{
  auto gridsize = Constants::kGridSizePxF;
  using SystemStoreType = Sys::Store::Type;

  auto &m_persistent_sys = m_sys.find<SystemStoreType::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();

  m_scene_data = std::make_shared<SceneData>( "res/scenes/ruinlower.json" );

  auto scene_settings_entt = m_reg.create();
  m_reg.emplace_or_replace<Cmp::SceneSettings::CurrentScene>( scene_settings_entt, Cmp::SceneSettings::SceneId::RUIN_LOWER_FLOOR );
  m_reg.emplace_or_replace<Cmp::SceneSettings::CollisionDetection>( scene_settings_entt, true );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowPathFinding>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowNavmesh>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowDebugStats>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Shaders>( scene_settings_entt, true );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Footsteps>( scene_settings_entt, true );

  // initialise the persistent player start position from the scene data
  auto [_, player_start_pos_px] = m_scene_data->get_player_start_position();
  Sys::PersistSystem::add<Cmp::Persist::PlayerStartPosition>( m_reg, player_start_pos_px );

  auto [map_size_grid, map_size_pixel] = m_scene_data->map_size();

  Factory::Shader::add_night_static( m_sys.find<Sys::Store::Type::ShaderSystem>(), map_size_pixel );

  // Positions reserved from procgen/algorithmic changes. Must exist before generation starts.
  m_reserved_sm = std::make_shared<PathFinding::SpatialHashGrid>();
  m_sys.find<SystemStoreType::ItemSystem>().init( m_reserved_sm );

  // generate the empty game area
  auto &level_gen = m_sys.find<SystemStoreType::LevelGenerator>();
  level_gen.init( m_reserved_sm );
  level_gen.build_scene_from_data( *m_scene_data );
  level_gen.add_ruin_rune_markers();
  auto max_cobwebs = Sys::PersistSystem::get<Cmp::Persist::RuinMaxCobwebs>( m_reg ).get_value();
  level_gen.add_lowerfloor_cobwebs( max_cobwebs, sf::FloatRect( { 0.f, 0.f }, map_size_pixel ) );
  auto init_chance = Sys::PersistSystem::get<Cmp::Persist::RuinProcGenInitChance>( m_reg ).get_value();
  level_gen.add_ruin_interior_obstacles( init_chance );

  auto max_iterations = Sys::PersistSystem::get<Cmp::Persist::RuinProcGenMaxIterations>( m_reg );
  auto &dla_sys = m_sys.find<Sys::Store::Type::DiffusionLtdAggrSystem>();
  dla_sys.iterate( sf::FloatRect( { 0.f, 0.f }, map_size_pixel ), sf::Vector2f{ 224.f, 144.f }, max_iterations.get_value(),
                   level_gen.get_obstacle_sm() );

  level_gen.decorate_ruin_interior_obstacles();
  m_sys.find<Sys::Store::Type::RuinSystem>().remove_rune_markings_neighbouring_cobwebs( level_gen.get_non_obstacle_sm() );

  // add access hitbox just above horizontal centerpoint
  sf::Vector2f flooraccess_position( map_size_pixel.x - ( 3 * gridsize.x ), 2 * gridsize.y );
  sf::Vector2f flooraccess_size( ( 2 * gridsize.x ), gridsize.y );
  m_sys.find<SystemStoreType::RuinSystem>().spawn_floor_access( flooraccess_position, flooraccess_size, Cmp::Ruin::FloorAccess::Direction::TO_UPPER );

  Sprites::Containers::VertexFloor floortiles;
  floortiles.create( level_gen.get_void_sm(), m_scene_data );
  auto floor_entity = m_reg.create();
  m_reg.emplace<Sprites::Containers::VertexFloor>( floor_entity, floortiles );
  m_reg.emplace<Cmp::ZOrderValue>( floor_entity, -16.f );

  m_sys.find<Sys::Store::Type::RuinSystem>().reset_player_curse();

  // create navmeshes for pathfinding
  m_generic_npc_navmesh = Factory::Pathfinding::create_npc_navmesh( m_reg );
  m_open_navmesh = Factory::Pathfinding::create_open_navmesh( m_reg );
  reinit_navmesh();

  Factory::Player::add_inventory( m_reg, "item.candle" );
  Factory::Particle::add_flame_for_player_inventory_slot( m_reg );

  // Hide the sudden position update/camera pan behind a forced loading screen.
  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
}

void RuinSceneLowerFloor::on_enter()
{
  SPDLOG_DEBUG( "Entering {}", get_name() );

  auto &m_persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();

  m_sound_bank.get_music( "graveyard_music" ).stop();

  auto [_, inventory_type, _] = Utils::Player::get_inventory( m_reg );
  if ( inventory_type != "item.witchesjar" )
  {
    if ( m_sound_bank.get_music( "ruin_creaking_rope" ).getStatus() != sf::Sound::Status::Playing )
    {
      m_sound_bank.get_music( "ruin_creaking_rope" ).play();
      m_sound_bank.get_music( "ruin_creaking_rope" ).setLooping( true );
    }
    if ( m_sound_bank.get_music( "ruin_music" ).getStatus() != sf::Sound::Status::Playing ) { m_sound_bank.get_music( "ruin_music" ).play(); }
  }

  m_sys.find<Sys::Store::Type::RenderGameSystem>().init_world_view();

  // prevent residual lerp movements from previous scene causing havoc in the new one
  Utils::Player::remove_lerp_cmp( m_reg );

  // prevent the player from wandering off before the scene has loaded
  Utils::Player::get_direction( m_reg ) = Cmp::Direction{ { 0.f, 0.f } };

  auto &player_pos = Utils::Player::get_position( m_reg );
  switch ( m_entry_mode )
  {
    case EntryMode::FROM_DOOR: {
      if ( m_just_spawned )
      {
        SPDLOG_DEBUG( "Player entering from door" );
        player_pos.position = Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( m_reg );
        m_just_spawned = false;
      }
      break;
    }
    case EntryMode::FROM_UPPER_FLOOR: {
      SPDLOG_DEBUG( "Player entering from upper floor" );
      player_pos.position = Utils::snap_to_grid( player_pos.position );
      m_sys.find<Sys::Store::Type::PlayerSystem>().force_expire_damage_cooldown();
      break;
    }
  }
  SPDLOG_DEBUG( "Player entered RuinSceneLowerFloor at position ({}, {})", player_pos.position.x, player_pos.position.y );

  auto player_entt = Utils::Player::get_entity( m_reg );
  m_reg.emplace_or_replace<Cmp::Player::RuinLocation>( player_entt, Cmp::Player::RuinLocation::Floor::LOWER );

  m_sys.find<Sys::Store::Type::RuinSystem>().reset_floor_access_cooldown();

  // re-initialise the spatial grid weak_ptr for when we return from upper floor scene
  reinit_navmesh();

  // Set the default footstep SFX for this scene
  m_reg.emplace_or_replace<Cmp::Player::Footstep>( Utils::Player::get_entity( m_reg ), Cmp::Player::Footstep::Type::STONE );
}

void RuinSceneLowerFloor::on_exit()
{
  SPDLOG_DEBUG( "Exiting {}", get_name() );
  m_sound_bank.get_music( "ruin_creaking_rope" ).stop();
  m_sound_bank.get_music( "ruin_music" ).stop();
  m_sys.find<Sys::Store::Type::FootstepSystem>().stop_footsteps_sound();

  m_reg.clear();

  // Hide the sudden position update/camera pan behind a forced loading screen.
  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
}

void RuinSceneLowerFloor::do_update( [[maybe_unused]] sf::Time dt )
{
  using namespace Sys;
  m_sys.find<Store::Type::AnimSystem>().update( dt );
  m_sys.find<Store::Type::NpcSystem>().update( dt );
  m_sys.find<Sys::Store::Type::FootstepSystem>().update();
  m_sys.find<Store::Type::LootSystem>().check_loot_collision();
  m_sys.find<Store::Type::RuinSystem>().check_floor_access_collision( Cmp::Ruin::FloorAccess::Direction::TO_UPPER );
  m_sys.find<Store::Type::RuinSystem>().check_movement_slowdowns();
  m_sys.find<Store::Type::RuinSystem>().creaking_rope_update();
  m_sys.find<Store::Type::RuinSystem>().check_puzzle_status();
  m_sys.find<Store::Type::ActionSystem>().update( dt );

  m_sys.find<Store::Type::PlayerSystem>().update( dt );

  auto [_, map_size_pixel] = m_scene_data->map_size();

  bool is_player_cursed = m_sys.find<Sys::Store::Type::RuinSystem>().check_activate_player_curse( map_size_pixel );
  if ( is_player_cursed )
  {
    // m_sys.find<Store::Type::RuinSystem>().check_create_witch( m_reg, sf::FloatRect( { 0, 0 }, map_size_pixel ) );
    auto scene_dimensions = sf::FloatRect( { 0, 0 }, map_size_pixel );
    m_sys.find<Store::Type::RuinSystem>().create_spiders( scene_dimensions );
    // Factory::Shader::add_curse( m_sys.find<Sys::Store::Type::ShaderSystem>(), map_size_pixel );
  }

  // `check_exit_collision()` may reset the player curse so it must be called after `check_activate_player_curse()`
  // or we incorrectly re-trigger the curse effects before we can leave this function and exit the scene.
  // i.e. the witch scream triggers again as we leave the scene
  m_sys.find<Store::Type::RuinSystem>().check_exit_collision();

  m_sys.find<Sys::Store::Type::ParticleSystem>().update( dt );
  auto &overlay_sys = m_sys.find<Store::Type::RenderOverlaySystem>();
  m_sys.find<Store::Type::RenderGameSystem>().render_game( dt, overlay_sys, m_generic_npc_navmesh );
}

void RuinSceneLowerFloor::reinit_navmesh()
{
  m_sys.find<Sys::Store::Type::NpcSystem>().init( m_generic_npc_navmesh, m_open_navmesh );
  m_sys.find<Sys::Store::Type::PlayerSystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_open_navmesh );
  m_sys.find<Sys::Store::Type::RenderOverlaySystem>().init( m_generic_npc_navmesh, m_reserved_sm );
  m_sys.find<Sys::Store::Type::RuinSystem>().init( m_generic_npc_navmesh, m_reserved_sm );
}

entt::registry &RuinSceneLowerFloor::registry() { return m_reg; }

} // namespace Game::Scene