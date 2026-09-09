#include <Audio/SoundBank.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/Curse.hpp>
#include <Components/Player/FootstepType.hpp>
#include <Components/Player/RuinLocation.hpp>
#include <Components/Ruin/HexagramMultiBlock.hpp>
#include <Components/Ruin/HexagramSegment.hpp>
#include <Components/Ruin/ObjectiveType.hpp>
#include <Components/Ruin/StairsBalustradeMultiBlock.hpp>
#include <Components/Ruin/StairsUpperMultiBlock.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/SceneSettings/CurrentScene.hpp>
#include <Components/SceneSettings/Footsteps.hpp>
#include <Components/SceneSettings/Shaders.hpp>
#include <Components/SceneSettings/ShowDebugStats.hpp>
#include <Components/SceneSettings/ShowNavmesh.hpp>
#include <Components/SceneSettings/ShowPathFinding.hpp>

#include <Factory/MultiblockFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PathfindingFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Factory/RuinFactory.hpp>
#include <Factory/ShaderFactory.hpp>
#include <Factory/WallFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SceneControl/SceneData.hpp>
#include <SceneControl/Scenes/RuinSceneUpperFloor.hpp>
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
#include <Systems/RuinSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Systems/Threats/NpcSystem.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Audio/Sound.hpp>

namespace Game::Scene
{

void RuinSceneUpperFloor::on_init()
{
  using namespace Sys;
  auto gridsize = Constants::kGridSizePxF;

  auto &m_persistent_sys = m_sys.find<Store::Type::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();

  m_scene_data = std::make_shared<SceneData>( "res/scenes/ruinupper.json" );

  auto scene_settings_entt = m_reg.create();
  m_reg.emplace_or_replace<Cmp::SceneSettings::CurrentScene>( scene_settings_entt, Cmp::SceneSettings::SceneId::RUIN_UPPER_FLOOR );
  m_reg.emplace_or_replace<Cmp::SceneSettings::CollisionDetection>( scene_settings_entt, true );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowPathFinding>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowNavmesh>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowDebugStats>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Shaders>( scene_settings_entt, true );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Footsteps>( scene_settings_entt, true );

  // initialise the persistent player start position from the scene data
  auto [_, player_start_pos_px] = m_scene_data->get_player_start_position();
  PersistSystem::add<Cmp::Persist::PlayerStartPosition>( m_reg, player_start_pos_px );

  auto [map_size_grid, map_size_pixel] = m_scene_data->map_size();

  Factory::Shader::add_night_static( m_sys.find<Sys::Store::Type::ShaderSystem>(), map_size_pixel );

  // generate the empty game area
  sf::Vector2f player_start_pos = PersistSystem::get<Cmp::Persist::PlayerStartPosition>( m_reg );
  auto player_start_area = Cmp::RectBounds::scaled( player_start_pos, gridsize, 1.f, Cmp::RectBounds::ScaleAxis::XY );
  // Positions reserved from procgen/algorithmic changes. Must exist before generation starts.
  m_reserved_sm = std::make_shared<PathFinding::SpatialHashGrid>();
  m_sys.find<Store::Type::ItemSystem>().init( m_reserved_sm );

  auto &random_level_sys = m_sys.find<Store::Type::LevelGenerator>();
  random_level_sys.init( m_reserved_sm );
  random_level_sys.build_scene_from_data( *m_scene_data );

  // add access hitbox just below horizontal centerpoint
  sf::Vector2f flooraccess_position( map_size_pixel.x - ( 3 * gridsize.x ), map_size_pixel.y - ( 3 * gridsize.y ) );
  sf::Vector2f flooraccess_size( ( 2 * gridsize.x ), gridsize.y );
  m_sys.find<Store::Type::RuinSystem>().spawn_floor_access( flooraccess_position, flooraccess_size, Cmp::Ruin::FloorAccess::Direction::TO_LOWER );

  Sprites::Containers::VertexFloor floortiles;
  floortiles.create( random_level_sys.get_void_sm(), m_scene_data );
  auto floor_entity = m_reg.create();
  m_reg.emplace<Sprites::Containers::VertexFloor>( floor_entity, floortiles );
  m_reg.emplace<Cmp::ZOrderValue>( floor_entity, -16.f );

  // create navmeshes for pathfinding
  m_generic_npc_navmesh = Factory::Pathfinding::create_npc_navmesh( m_reg );
  m_open_navmesh = Factory::Pathfinding::create_open_navmesh( m_reg );
  reinit_navmesh();

  // Add a flame ParticleSprite for a candle in the player inventory
  Factory::Particle::add_flame_for_player_inventory_slot( m_reg );
}

void RuinSceneUpperFloor::on_enter()
{
  SPDLOG_DEBUG( "Entering {}", get_name() );

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
  auto &m_persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();

  m_sys.find<Sys::Store::Type::RenderGameSystem>().init_world_view();

  // prevent residual lerp movements from previous scene causing havoc in the new one
  Utils::Player::remove_lerp_cmp( m_reg );

  // prevent the player from wandering off before the scene has loaded
  auto &player_dir = Utils::Player::get_direction( m_reg );
  player_dir = Cmp::Direction{ { 0.f, 0.f } };

  if ( m_just_spawned )
  {
    auto &player_pos = Utils::Player::get_position( m_reg );
    player_pos.position = Utils::snap_to_grid( player_pos.position );
    m_just_spawned = false;
  }

  auto player_entt = Utils::Player::get_entity( m_reg );
  m_reg.emplace_or_replace<Cmp::Player::RuinLocation>( player_entt, Cmp::Player::RuinLocation::Floor::UPPER );

  m_sys.find<Sys::Store::Type::RuinSystem>().reset_floor_access_cooldown();

  // Set the default footstep SFX for this scene
  m_reg.emplace_or_replace<Cmp::Player::Footstep>( Utils::Player::get_entity( m_reg ), Cmp::Player::Footstep::Type::WOODFLOOR );
}

void RuinSceneUpperFloor::on_exit()
{
  SPDLOG_DEBUG( "Exiting {}", get_name() );
  m_sound_bank.get_music( "ruin_creaking_rope" ).stop();
  m_sound_bank.get_music( "ruin_music" ).stop();
  m_reg.clear();
}

void RuinSceneUpperFloor::do_update( sf::Time dt )
{
  using namespace Sys;
  m_sys.find<Store::Type::AnimSystem>().update( dt );
  m_sys.find<Store::Type::NpcSystem>().update( dt );
  m_sys.find<Sys::Store::Type::FootstepSystem>().update();
  m_sys.find<Store::Type::LootSystem>().check_loot_collision();
  m_sys.find<Store::Type::RuinSystem>().check_floor_access_collision( Cmp::Ruin::FloorAccess::Direction::TO_LOWER );
  m_sys.find<Store::Type::RuinSystem>().check_movement_slowdowns();

  m_sys.find<Store::Type::PlayerSystem>().update( dt );
  m_sys.find<Store::Type::PlayerSystem>().disable_damage_cooldown();

  auto [_, map_size_pixel] = m_scene_data->map_size();
  [[maybe_unused]] bool player_curse_active = m_sys.find<Store::Type::RuinSystem>().check_activate_player_curse( map_size_pixel );
  // if ( player_curse_active ) { Factory::Shader::add_curse( m_sys.find<Sys::Store::Type::ShaderSystem>(), map_size_pixel ); }

  m_sys.find<Store::Type::RuinSystem>().update_shadow_hand_pos( map_size_pixel );
  m_sys.find<Store::Type::RuinSystem>().check_player_shadow_hand_collision( dt );

  m_sys.find<Sys::Store::Type::ParticleSystem>().update( dt );
  auto &overlay_sys = m_sys.find<Store::Type::RenderOverlaySystem>();
  m_sys.find<Store::Type::RenderGameSystem>().render_game( dt, overlay_sys, m_generic_npc_navmesh );
}

void RuinSceneUpperFloor::reinit_navmesh()
{
  m_sys.find<Sys::Store::Type::NpcSystem>().init( m_generic_npc_navmesh, m_open_navmesh );
  m_sys.find<Sys::Store::Type::PlayerSystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_open_navmesh );
  m_sys.find<Sys::Store::Type::RenderOverlaySystem>().init( m_generic_npc_navmesh, m_reserved_sm );
}

entt::registry &RuinSceneUpperFloor::registry() { return m_reg; }

} // namespace Game::Scene