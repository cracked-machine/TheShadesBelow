#include <Audio/SoundBank.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/Character.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/SceneSettings/CurrentScene.hpp>
#include <Components/SceneSettings/Footsteps.hpp>
#include <Components/SceneSettings/Shaders.hpp>
#include <Components/SceneSettings/ShowDebugStats.hpp>
#include <Components/SceneSettings/ShowNavmesh.hpp>
#include <Components/SceneSettings/ShowPathFinding.hpp>

#include <Factory/MultiblockFactory.hpp>
#include <Factory/PathfindingFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SceneControl/SceneData.hpp>
#include <SceneControl/Scenes/HealingSpringScene.hpp>
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
#include <Systems/ProcGen/LevelGenerator.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/Render/RenderOverlaySystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Systems/Threats/NpcSystem.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

namespace Game::Scene
{

void HealingSpringScene::on_init()
{
  auto &m_persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();

  m_scene_data = std::make_shared<SceneData>( "res/scenes/well.json" );
  SPDLOG_DEBUG( "wall_tilelayer size: {}", m_scene_data->wall_tilelayer().size() );

  auto scene_settings_entt = m_reg.create();
  m_reg.emplace_or_replace<Cmp::SceneSettings::CurrentScene>( scene_settings_entt, Cmp::SceneSettings::SceneId::HEALING_SPRING );
  m_reg.emplace_or_replace<Cmp::SceneSettings::CollisionDetection>( scene_settings_entt, true );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowPathFinding>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowNavmesh>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowDebugStats>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Shaders>( scene_settings_entt, true );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Footsteps>( scene_settings_entt, true );

  // initialise the persistent player start position from the scene configuration (json) data
  auto [_, player_start_pos_px] = m_scene_data->get_player_start_position();
  Sys::PersistSystem::add<Cmp::Persist::PlayerStartPosition>( m_reg, player_start_pos_px );

  // create the empty game area
  sf::Vector2f player_start_position = Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( m_reg );
  auto player_start_area = Cmp::RectBounds::scaled( player_start_position, Constants::kGridSizePxF, 1.f, Cmp::RectBounds::ScaleAxis::XY );
  // Positions reserved from procgen/algorithmic changes. Must exist before generation starts.
  m_reserved_sm = std::make_shared<PathFinding::SpatialHashGrid>();
  m_sys.find<Sys::Store::Type::ItemSystem>().init( m_reserved_sm );

  auto &random_level_sys = m_sys.find<Sys::Store::Type::LevelGenerator>();
  random_level_sys.init( m_reserved_sm );
  random_level_sys.build_scene_from_data( *m_scene_data );

  Sprites::Containers::VertexFloor floortiles;
  floortiles.create( random_level_sys.get_void_sm(), m_scene_data );
  auto floor_entity = m_reg.create();
  m_reg.emplace<Sprites::Containers::VertexFloor>( floor_entity, floortiles );
  m_reg.emplace<Cmp::ZOrderValue>( floor_entity, -16.f );

  // create navmeshes for pathfinding
  m_generic_npc_navmesh = Factory::Pathfinding::create_npc_navmesh( m_reg );
  m_open_navmesh = Factory::Pathfinding::create_open_navmesh( m_reg );
  reinit_navmesh();

  // Hide the sudden position update/camera pan behind a forced loading screen.
  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
}

void HealingSpringScene::on_enter()
{
  SPDLOG_DEBUG( "Entering {}", get_name() );
  m_sound_bank.get_music( "graveyard_music" ).stop();
  if ( m_sound_bank.get_music( "healing_spring" ).getStatus() != sf::Sound::Status::Playing )
  {
    m_sound_bank.get_music( "healing_spring" ).play();
    m_sound_bank.get_music( "healing_spring" ).setLooping( true );
  }

  auto &m_persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();

  m_sys.find<Sys::Store::Type::RenderGameSystem>().init_world_view();

  if ( m_just_spawned )
  {
    auto &player_pos = Utils::Player::get_position( m_reg );
    player_pos.position = Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( m_reg );
    m_just_spawned = false;
  }

  // prevent the player from wandering off before the scene has loaded
  auto &player_dir = Utils::Player::get_direction( m_reg );
  player_dir = Cmp::Direction{ { 0.f, 0.f } };

  // no blink/cooldown in the healing spring is required
  m_sys.find<Sys::Store::Type::PlayerSystem>().force_expire_damage_cooldown();

  // Set the default footstep SFX for this scene
  m_reg.emplace_or_replace<Cmp::Player::Footstep>( Utils::Player::get_entity( m_reg ), Cmp::Player::Footstep::Type::STONE );
}

void HealingSpringScene::on_exit()
{
  SPDLOG_DEBUG( "Exiting {}", get_name() );
  m_sound_bank.get_music( "healing_spring" ).stop();
  m_sys.find<Sys::Store::Type::FootstepSystem>().stop_footsteps_sound();
  m_reg.clear();

  // Hide the sudden position update/camera pan behind a forced loading screen.
  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
}

void HealingSpringScene::do_update( sf::Time dt )
{
  m_sys.find<Sys::Store::Type::AnimSystem>().update( dt );
  m_sys.find<Sys::Store::Type::NpcSystem>().update( dt );
  m_sys.find<Sys::Store::Type::FootstepSystem>().update();
  m_sys.find<Sys::Store::Type::LootSystem>().check_loot_collision();
  m_sys.find<Sys::Store::Type::HealingSpringSystem>().check_exit_collision();
  // m_sys.find<Sys::Store::Type::HealingSpringSystem>().check_inventory_deposit( dt );

  m_sys.find<Sys::Store::Type::PlayerSystem>().update( dt );
  m_sys.find<Sys::Store::Type::ParticleSystem>().update( dt );

  auto &overlay_sys = m_sys.find<Sys::Store::Type::RenderOverlaySystem>();
  m_sys.find<Sys::Store::Type::RenderGameSystem>().render_game( dt, overlay_sys, m_generic_npc_navmesh );
}

void HealingSpringScene::reinit_navmesh()
{
  m_sys.find<Sys::Store::Type::NpcSystem>().init( m_generic_npc_navmesh, m_open_navmesh );
  m_sys.find<Sys::Store::Type::PlayerSystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_open_navmesh );
  m_sys.find<Sys::Store::Type::RenderOverlaySystem>().init( m_generic_npc_navmesh, m_reserved_sm );
}

entt::registry &HealingSpringScene::registry() { return m_reg; }

} // namespace Game::Scene