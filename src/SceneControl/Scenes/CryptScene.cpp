
#include <Audio/SoundBank.hpp>
#include <Components/Crypt/RoomStart.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Particle/FlameParticleSprite.hpp>
#include <Components/Particle/LavaFireballParticleSprite.hpp>
#include <Components/Particle/NoteParticleSprite.hpp>
#include <Components/Persistent/CryptVariantOverride.hpp>
#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/FootstepType.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/SceneSettings/CurrentScene.hpp>
#include <Components/SceneSettings/Footsteps.hpp>
#include <Components/SceneSettings/Paused.hpp>
#include <Components/SceneSettings/Shaders.hpp>
#include <Components/SceneSettings/ShowDebugStats.hpp>
#include <Components/SceneSettings/ShowNavmesh.hpp>
#include <Components/SceneSettings/ShowPathFinding.hpp>
#include <Events/UpdatePlayerPositionEvent.hpp>
#include <Factory/CryptFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PathfindingFactory.hpp>
#include <Factory/ShaderFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SceneControl/SceneData.hpp>
#include <SceneControl/Scenes/CryptScene.hpp>
#include <Sprites/VertexFloor.hpp>
#include <Systems/AnimSystem.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Crypt/Variants/CryptVariant.hpp>
#include <Systems/Crypt/Variants/CryptVariantArbiter.hpp>
#include <Systems/CryptSystem.hpp>
#include <Systems/FootstepSystem.hpp>
#include <Systems/InventorySystem.hpp>
#include <Systems/ItemSystem.hpp>
#include <Systems/LootSystem.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/PlayerSystem.hpp>
#include <Systems/ProcGen/DowsingGuideSystem.hpp>
#include <Systems/ProcGen/LevelGenerator.hpp>
#include <Systems/ProcGen/PassageSystem.hpp>
#include <Systems/Render/PauseOverlaySystem.hpp>
#include <Systems/Render/DebugOverlaySystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/Render/OverlayArbitrationSystem.hpp>
#include <Systems/ShaderSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Systems/Threats/NpcSystem.hpp>
#include <Systems/Threats/ShockwaveSystem.hpp>
#include <Utils/Player.hpp>

namespace Game::Scene
{

CryptScene::CryptScene( Audio::SoundBank &sound_bank, Sys::Store &system_store, entt::dispatcher &nav_event_dispatcher )
    : Scene( nav_event_dispatcher ),
      m_sound_bank( sound_bank ),
      m_sys( system_store )
{
}

CryptScene::~CryptScene() = default;

void CryptScene::on_init()
{

  auto &m_persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();

  m_scene_data = std::make_shared<SceneData>( "res/scenes/crypt.json" );
  SPDLOG_DEBUG( "get_floor_image: {}", m_scene_data->floor_tileset_image().string() );
  SPDLOG_DEBUG( "levelgen_tilelayer size: {}", m_scene_data->levelgen_tilelayer().size() );

  auto scene_settings_entt = m_reg.create();
  m_reg.emplace_or_replace<Cmp::SceneSettings::CurrentScene>( scene_settings_entt, Cmp::SceneSettings::SceneId::CRYPT );
  m_reg.emplace_or_replace<Cmp::SceneSettings::CollisionDetection>( scene_settings_entt, true );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowPathFinding>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowNavmesh>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowDebugStats>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Paused>( scene_settings_entt, Cmp::SceneSettings::Paused::Reason::NONE );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Shaders>( scene_settings_entt, true );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Footsteps>( scene_settings_entt, true );

  // initialise the persistent player start position from the scene configuration (json) data
  auto [_, player_start_pos_px] = m_scene_data->get_player_start_position();
  SPDLOG_DEBUG( "player start position {},{}", player_start_pos_px.x, player_start_pos_px.y );
  Sys::PersistSystem::add<Cmp::Persist::PlayerStartPosition>( m_reg, player_start_pos_px );

  auto [map_size_grid, map_size_pixel] = m_scene_data->map_size();

  Factory::Shader::add_night_static( m_sys.find<Sys::Store::Type::ShaderSystem>(), map_size_pixel );

  // Positions reserved from procgen/algorithmic changes. Must exist before generation starts,
  // since room/interior carving and passage caching below query and populate it.
  m_reserved_sm = std::make_shared<PathFinding::SpatialHashGrid>();
  m_item_sm = std::make_shared<PathFinding::SpatialHashGrid>();
  m_sys.find<Sys::Store::Type::CryptSystem>().init( nullptr, nullptr, m_reserved_sm, nullptr, m_item_sm );
  m_sys.find<Sys::Store::Type::PassageSystem>().init_reserved_sm( m_reserved_sm );
  m_sys.find<Sys::Store::Type::ItemSystem>().init( m_reserved_sm, m_item_sm );

  // create the empty game area
  auto player_start_area = m_scene_data->get_spawn_area_bounds();
  auto &random_level_sys = m_sys.find<Sys::Store::Type::LevelGenerator>();
  random_level_sys.init( m_reserved_sm, m_item_sm );
  random_level_sys.build_scene_from_data( *m_scene_data );
  auto &crypt_sys = m_sys.find<Sys::Store::Type::CryptSystem>();
  auto &passage_sys = m_sys.find<Sys::Store::Type::PassageSystem>();
  passage_sys.init_scene_data( m_scene_data );

  // pick this crypt's design
  const int variant_override = Sys::PersistSystem::get<Cmp::Persist::CryptVariantOverride>( m_reg ).get_value();
  m_variant = Sys::Crypt::CryptVariantArbiter::with_default_variants().pick( crypt_sys, passage_sys, variant_override );
  passage_sys.set_config( m_variant->passage_config() );
  crypt_sys.set_variant( m_variant.get() );

  // intialise the game area
  auto start_room_entity = m_reg.create();
  m_reg.emplace_or_replace<Cmp::Crypt::RoomStart>( start_room_entity, player_start_area.position, player_start_area.size );
  m_variant->generate( map_size_grid );

  Factory::Shader::add_red_vignette( m_sys.find<Sys::Store::Type::ShaderSystem>(),
                                     Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( m_reg ) );
  Factory::Shader::add_screen_shudder( m_sys.find<Sys::Store::Type::ShaderSystem>(),
                                       Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( m_reg ) );

  // create navmeshes for pathfinding
  m_generic_npc_navmesh = Factory::Pathfinding::create_npc_navmesh( m_reg );
  m_open_navmesh = Factory::Pathfinding::create_open_navmesh( m_reg );
  m_loot_sm = Factory::Pathfinding::create_loot_grid( m_reg );
  m_player_navmesh = Factory::Pathfinding::create_player_navmesh( m_reg );
  reinit_navmesh();

  Sprites::Containers::VertexFloor floortiles;
  floortiles.create( random_level_sys.get_void_sm(), m_scene_data );
  auto floor_entity = m_reg.create();
  m_reg.emplace<Sprites::Containers::VertexFloor>( floor_entity, floortiles );
  m_reg.emplace<Cmp::ZOrderValue>( floor_entity, -16.f );

  // The player has not been moved to the start position yet, so take the flame zorder from the torch (not the player).
  for ( auto [worlditem_entt, worlditem_cmp, worlditem_pos_cmp, worlditem_uuid_cmp, worlditem_zorder_cmp] :
        m_reg.view<Cmp::WorldItem, Cmp::Position, Cmp::UUID, Cmp::ZOrderValue>().each() )
  {
    if ( worlditem_cmp.item_type != "item.torch" ) continue;
    Factory::Particle::add_flame(
        m_reg, "crypt.torch.particle.flame", worlditem_uuid_cmp,
        { worlditem_pos_cmp.getCenter().x, worlditem_pos_cmp.getCenter().y - Cmp::Particle::FlameParticleSprite::kVerticalOffset },
        worlditem_zorder_cmp.get() + 1.f, Cmp::Particle::kWorldScalePreset );
  }
}

void CryptScene::on_enter()
{
  // Initialize entities specific to the CryptScene
  SPDLOG_DEBUG( "Entering {}", get_name() );

  auto &m_persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();

  m_sys.find<Sys::Store::Type::RenderGameSystem>().init_world_view();

  // the player is moved through PlayerSystem, which needs this scene's navmeshes to keep them in sync
  reinit_navmesh();
  m_sys.find<Sys::Store::Type::CryptSystem>().set_variant( m_variant.get() );

  if ( m_just_spawned )
  {
    Sys::BaseSystem::get_systems_event_queue().trigger(
        Events::UpdatePlayerPositionEvent( sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( m_reg ) ) ) );
    m_just_spawned = false;
    if ( Utils::Player::get_mortality( m_reg ).state != Cmp::Player::Mortality::State::DEAD ) m_variant->on_first_enter();
  }

  // prevent the player from wandering off before the scene has loaded
  auto &player_dir = Utils::Player::get_direction( m_reg );
  player_dir = Cmp::Direction{ { 0.f, 0.f } };

  // Set the default footstep SFX for this scene
  m_reg.emplace_or_replace<Cmp::Player::Footstep>( Utils::Player::get_entity( m_reg ), Cmp::Player::Footstep::Type::STONE );

  // check if the player inventory has a torch, if so light it up!
  Factory::Particle::add_flame_for_player_inventory_slot( m_reg );

  m_sound_bank.stop_all();

  // Hide the sudden position update/camera pan behind a forced loading screen.
  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
}

void CryptScene::on_exit()
{
  // never leave the scene paused, e.g. with its sound effects still paused
  m_sys.find<Sys::Store::Type::PauseOverlaySystem>().force_resume();
  // Cleanup any resources or entities specific to the CryptScene
  SPDLOG_DEBUG( "Exiting {}", get_name() );
  Factory::Crypt::destroy_crypt_shuffle_timer( m_reg );
  m_sys.find<Sys::Store::Type::CryptSystem>().set_variant( nullptr );
  m_sys.find<Sys::Store::Type::FootstepSystem>().stop_footsteps_sound();

  // Hide the sudden position update/camera pan behind a forced loading screen.
  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
}

void CryptScene::do_update( sf::Time dt )
{
  // the pause menu and pausing tooltips pause gameplay: the world is still drawn, but nothing in it advances
  const bool paused = Sys::OverlayArbitrationSystem::active_modal( m_reg ) != Sys::OverlayArbitrationSystem::Modal::NONE;
  const sf::Time world_dt = paused ? sf::Time::Zero : dt;

  if ( not paused )
  {
    m_sys.find<Sys::Store::Type::AnimSystem>().update( dt );
    m_sys.find<Sys::Store::Type::NpcSystem>().update( dt );
    m_sys.find<Sys::Store::Type::FootstepSystem>().update( dt );
    m_sys.find<Sys::Store::Type::LootSystem>().check_loot_collision( dt );
    m_sys.find<Sys::Store::Type::CryptSystem>().update( dt );
    m_sys.find<Sys::Store::Type::ShockwaveSystem>().update( dt );
    m_sys.find<Sys::Store::Type::PlayerSystem>().update( dt );
    m_sys.find<Sys::Store::Type::ShaderSystem>().update( dt );
    m_sys.find<Sys::Store::Type::PassageSystem>().update( dt );
    m_sys.find<Sys::Store::Type::DowsingGuideSystem>().update( dt );
    m_sys.find<Sys::Store::Type::InventorySystem>().update( dt );

    // lava fireballs arc over the walls rather than colliding with them
    auto excludes = { std::string( Cmp::Particle::NoteParticleSprite::kPlayerNotesTag ),
                      std::string( Cmp::Particle::LavaFireballParticleSprite::kLavaPitTag ) };
    m_sys.find<Sys::Store::Type::ParticleSystem>().check_collsion( excludes );
    m_sys.find<Sys::Store::Type::ParticleSystem>().update( dt );
  }

  m_sys.find<Sys::Store::Type::RenderGameSystem>().render_game( world_dt );
  // HUD, shop, tooltip, grimoire, pause and debug overlays, in one shared ImGui frame. Real dt even while paused, for ImGui.
  m_sys.find<Sys::Store::Type::OverlayArbitrationSystem>().update( dt, world_dt );
}

void CryptScene::reinit_navmesh()
{
  m_sys.find<Sys::Store::Type::NpcSystem>().init( m_generic_npc_navmesh, m_open_navmesh, nullptr, nullptr, m_loot_sm, m_item_sm );
  m_sys.find<Sys::Store::Type::ItemSystem>().init( m_reserved_sm, m_item_sm, m_generic_npc_navmesh, nullptr, m_loot_sm );
  m_sys.find<Sys::Store::Type::InventorySystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_reserved_sm, m_item_sm );
  m_sys.find<Sys::Store::Type::PassageSystem>().init_nav_mesh( m_generic_npc_navmesh );
  m_sys.find<Sys::Store::Type::DowsingGuideSystem>().init( m_generic_npc_navmesh );
  m_sys.find<Sys::Store::Type::CryptSystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_reserved_sm, m_loot_sm, m_item_sm );
  m_sys.find<Sys::Store::Type::LootSystem>().init( m_generic_npc_navmesh, m_player_navmesh, nullptr, m_reserved_sm, m_loot_sm, m_item_sm );
  m_sys.find<Sys::Store::Type::PlayerSystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_open_navmesh );
  m_sys.find<Sys::Store::Type::DebugOverlaySystem>().init( m_generic_npc_navmesh, m_reserved_sm, m_item_sm, nullptr );
}

entt::registry &CryptScene::registry() { return m_reg; }

} // namespace Game::Scene