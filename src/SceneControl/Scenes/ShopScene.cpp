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
#include <Components/Shop/Inventory.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PathfindingFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SceneControl/Events/ProcessShopSceneInputEvent.hpp>
#include <SceneControl/SceneData.hpp>
#include <SceneControl/Scenes/ShopScene.hpp>
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
#include <Systems/ShopSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Systems/Threats/NpcSystem.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

namespace Game::Scene
{

void ShopScene::on_init()
{
  auto &m_persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();

  m_scene_data = std::make_shared<SceneData>( "res/scenes/shop.json" );
  m_sys.find<Sys::Store::Type::ShopSystem>().create_shop_inventory();

  auto scene_settings_entt = m_reg.create();
  m_reg.emplace_or_replace<Cmp::SceneSettings::CurrentScene>( scene_settings_entt, Cmp::SceneSettings::SceneId::SHOP );
  m_reg.emplace_or_replace<Cmp::SceneSettings::CollisionDetection>( scene_settings_entt, true );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowPathFinding>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowNavmesh>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowDebugStats>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Shaders>( scene_settings_entt, true );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Footsteps>( scene_settings_entt, true );

  // initialise the persistent player start position from the scene configuration (json) data
  auto [_, player_start_pos_px] = m_scene_data->get_player_start_position();
  Sys::PersistSystem::add<Cmp::Persist::PlayerStartPosition>( m_reg, player_start_pos_px );
  SPDLOG_DEBUG( "player_start_pos_px: {},{}", player_start_pos_px.x, player_start_pos_px.y );

  // create the empty game area
  sf::Vector2f player_start_position = Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( m_reg );
  auto player_start_area = Cmp::RectBounds::scaled( player_start_position, Constants::kGridSizePxF, 1.f, Cmp::RectBounds::ScaleAxis::XY );
  // Positions reserved from procgen/algorithmic changes. Must exist before generation starts.
  m_reserved_sm = std::make_shared<PathFinding::SpatialHashGrid>();
  m_sys.find<Sys::Store::Type::ItemSystem>().init( m_reserved_sm );

  auto &random_level_sys = m_sys.find<Sys::Store::Type::LevelGenerator>();
  random_level_sys.init( m_reserved_sm );
  random_level_sys.build_scene_from_data( *m_scene_data );

  m_floormap.create( random_level_sys.get_void_sm(), m_scene_data );
  auto floor_entity = m_reg.create();
  m_reg.emplace<Sprites::Containers::VertexFloor>( floor_entity, m_floormap );
  m_reg.emplace<Cmp::ZOrderValue>( floor_entity, -16.f );

  // create navmeshes for pathfinding
  m_generic_npc_navmesh = Factory::Pathfinding::create_npc_navmesh( m_reg );
  m_open_navmesh = Factory::Pathfinding::create_open_navmesh( m_reg );
  reinit_navmesh();

  // prevent the player from wandering off before the scene has loaded
  auto &player_dir = Utils::Player::get_direction( m_reg );
  player_dir = Cmp::Direction{ { 0.f, 0.f } };

  // Hide the sudden position update/camera pan behind a forced loading screen.
  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
}

void ShopScene::on_enter()
{
  SPDLOG_DEBUG( "Entering {}", get_name() );
  m_sound_bank.get_music( "graveyard_music" ).stop();

  auto &m_persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();

  m_sys.find<Sys::Store::Type::RenderGameSystem>().init_world_view();

  if ( m_just_spawned )
  {
    auto &player_pos = Utils::Player::get_position( m_reg );
    player_pos.position = Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( m_reg );
    SPDLOG_DEBUG( "player_start_pos_px: {},{}", player_pos.position.x, player_pos.position.y );
    m_just_spawned = false;
  }

  // check if the player inventory has a candle, if so light it up!
  Factory::Particle::add_flame_for_player_inventory_slot( m_reg );

  // Set the default footstep SFX for this scene
  m_reg.emplace_or_replace<Cmp::Player::Footstep>( Utils::Player::get_entity( m_reg ), Cmp::Player::Footstep::Type::WOODFLOOR );
}

void ShopScene::on_exit()
{
  SPDLOG_DEBUG( "Exiting {}", get_name() );
  m_reg.clear();

  m_sys.find<Sys::Store::Type::FootstepSystem>().stop_footsteps_sound();

  // Hide the sudden position update/camera pan behind a forced loading screen.
  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
}

void ShopScene::do_update( [[maybe_unused]] sf::Time dt )
{
  // m_sys.find<Sys::Store::Type::HealingSpringSystem>().check_inventory_deposit( dt );
  m_sys.find<Sys::Store::Type::AnimSystem>().update( dt );
  m_sys.find<Sys::Store::Type::FootstepSystem>().update();
  m_sys.find<Sys::Store::Type::ShopSystem>().check_exit_collision();
  m_sys.find<Sys::Store::Type::ParticleSystem>().update( dt );

  if ( m_scene_data )
  {
    for ( const auto &[ms_type, pos] : m_scene_data->multiblock_objectlayer() )
    {
      if ( ms_type != "npc.drknox" ) continue;
      if ( m_sys.find<Sys::Store::Type::ShopSystem>().check_shopkeeper_collision( pos ) and not is_overlay_open() ) { open_overlay(); }
      else if ( not m_sys.find<Sys::Store::Type::ShopSystem>().check_shopkeeper_collision( pos ) ) { close_overlay(); }
    }
  }
  else { SPDLOG_WARN( "m_scene_map_data is not initialised" ); }

  m_sys.find<Sys::Store::Type::PlayerSystem>().update( dt );

  auto &overlay_sys = m_sys.find<Sys::Store::Type::RenderOverlaySystem>();
  m_sys.find<Sys::Store::Type::RenderGameSystem>().render_game( dt, overlay_sys, m_generic_npc_navmesh );
}

void ShopScene::open_overlay()
{
  auto inventory_view = m_reg.view<Cmp::Shop::Inventory>().each();
  for ( auto [inventory_entt, inventory_cmp] : inventory_view )
  {
    inventory_cmp.is_enabled = true;
  }
  m_overlay_open = true;
}

void ShopScene::close_overlay()
{
  auto inventory_view = m_reg.view<Cmp::Shop::Inventory>().each();
  for ( auto [inventory_entt, inventory_cmp] : inventory_view )
  {
    inventory_cmp.is_enabled = false;
  }
  m_overlay_open = false;
}

void ShopScene::reinit_navmesh()
{
  m_sys.find<Sys::Store::Type::NpcSystem>().init( m_generic_npc_navmesh, m_open_navmesh );
  m_sys.find<Sys::Store::Type::PlayerSystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_open_navmesh );
  m_sys.find<Sys::Store::Type::RenderOverlaySystem>().init( m_generic_npc_navmesh, m_reserved_sm );
}

entt::registry &ShopScene::registry() { return m_reg; }

} // namespace Game::Scene