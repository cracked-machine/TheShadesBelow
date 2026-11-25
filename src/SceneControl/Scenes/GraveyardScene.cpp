#include <Audio/SoundBank.hpp>
#include <Components/Cart/Barrel.hpp>
#include <Components/Direction.hpp>
#include <Components/Particle/EmberParticleSprite.hpp>
#include <Components/Particle/NoteParticleSprite.hpp>
#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Persistent/GraveyardProcGenInitChance.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/FootstepType.hpp>
#include <Components/Player/LastGraveyardPosition.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Random.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/SceneSettings/CurrentScene.hpp>
#include <Components/SceneSettings/Footsteps.hpp>
#include <Components/SceneSettings/Paused.hpp>
#include <Components/SceneSettings/Shaders.hpp>
#include <Components/SceneSettings/ShowDebugStats.hpp>
#include <Components/SceneSettings/ShowNavmesh.hpp>
#include <Components/SceneSettings/ShowPathFinding.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/UpdatePlayerPositionEvent.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PathfindingFactory.hpp>
#include <Factory/PlantFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Factory/ShaderFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SceneControl/SceneData.hpp>
#include <SceneControl/Scenes/GraveyardScene.hpp>
#include <Sprites/VertexFloor.hpp>
#include <Systems/ActionSystem.hpp>
#include <Systems/AltarSystem.hpp>
#include <Systems/AnimSystem.hpp>
#include <Systems/ArrowSystem.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/BurrSystem.hpp>
#include <Systems/CryptSystem.hpp>
#include <Systems/FlyingSystem.hpp>
#include <Systems/FootstepSystem.hpp>
#include <Systems/GraveSystem.hpp>
#include <Systems/HealingSpringSystem.hpp>
#include <Systems/InventorySystem.hpp>
#include <Systems/ItemSystem.hpp>
#include <Systems/LootSystem.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/PlantSystem.hpp>
#include <Systems/PlayerSystem.hpp>
#include <Systems/ProcGen/CellAutomataSystem.hpp>
#include <Systems/ProcGen/DowsingGuideSystem.hpp>
#include <Systems/ProcGen/LevelGenerator.hpp>
#include <Systems/Render/DebugOverlaySystem.hpp>
#include <Systems/Render/OverlayArbitrationSystem.hpp>
#include <Systems/Render/PauseOverlaySystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/RuinSystem.hpp>
#include <Systems/ShaderSystem.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Systems/TeleportSystem.hpp>
#include <Systems/Threats/BombSystem.hpp>
#include <Systems/Threats/HazardFieldSystemImpl.hpp>
#include <Systems/Threats/LightningSystem.hpp>
#include <Systems/Threats/WatchmanSystem.hpp>
#include <Systems/Threats/WispSystem.hpp>
#include <Systems/Threats/WormholeSystem.hpp>
#include <Utils/Player.hpp>
#include <Utils/Profiling.hpp>
#include <algorithm>

#include <array>
#include <cmath>
#include <string_view>
#include <utility>
#include <optional>
#include <vector>

namespace Game::Scene
{

void GraveyardScene::on_init()
{
  SPDLOG_DEBUG( "Init {}", get_name() );

  m_sys.find<Sys::Store::Type::PersistSystem>().initialize_component_registry();
  m_sys.find<Sys::Store::Type::PersistSystem>().load_state();

  auto scene_settings_entt = m_reg.create();
  m_reg.emplace_or_replace<Cmp::SceneSettings::CurrentScene>( scene_settings_entt, Cmp::SceneSettings::SceneId::GRAVEYARD );
  m_reg.emplace_or_replace<Cmp::SceneSettings::CollisionDetection>( scene_settings_entt, true );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowPathFinding>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowNavmesh>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowDebugStats>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Paused>( scene_settings_entt, Cmp::SceneSettings::Paused::Reason::NONE );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Shaders>( scene_settings_entt, true );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Footsteps>( scene_settings_entt, true );

  m_scene_data = std::make_shared<SceneData>( "res/scenes/graveyard.json" );

  // player start position
  auto [_, player_start_pos_px] = m_scene_data->get_player_start_position();
  Sys::PersistSystem::add<Cmp::Persist::PlayerStartPosition>( m_reg, player_start_pos_px );

  // Positions reserved from procgen/algorithmic changes. Must exist before anything below that
  // creates or queries reserved entities (ItemSystem is wired up now since its
  // events/calls can fire mid-generation, before reinit_system_spatial_maps() runs).
  m_reserved_sm = std::make_shared<PathFinding::SpatialHashGrid>();
  // ground items are indexed as they're created (level-gen item markers included); hazard cells as the fields spread
  m_item_sm = std::make_shared<PathFinding::SpatialHashGrid>();
  m_hazard_sm = std::make_shared<PathFinding::SpatialHashGrid>();
  m_sys.find<Sys::Store::Type::ItemSystem>().init( m_reserved_sm, m_item_sm );
  m_sys.find<Sys::Store::Type::PlantSystem>().init( m_reserved_sm, nullptr, m_item_sm, m_hazard_sm );

  // create the player, or increment the level counter for the new level
  if ( m_reg.view<Cmp::Player::Character>().size() == 0 )
  {
    Factory::Player::create_player( m_reg );
    // Sys::BaseSystem::get_systems_event_queue().trigger( Events::AddInventoryEvent( "item.shovel" ) );
  }
  else
  {
    Utils::Player::get_level_depth( m_reg ).increment_count( 1 );
    // every level starts with the Cmp::Persist::Starting* stats
    Utils::Player::reset_stats_to_starting( m_reg );
  }

  // add the number of cart barrels for this level
  int barrels = static_cast<int>( Utils::Player::get_level_depth( m_reg ).get_count() );
  for ( int _ : std::views::iota( 0, barrels ) )
  {
    auto barrel_entt = m_reg.create();
    m_reg.emplace<Cmp::Cart::Barrel>( barrel_entt );
  }

  auto [map_size_grid, map_size_pixel] = m_scene_data->map_size();

  Factory::Shader::add_water( m_sys.find<Sys::Store::Type::ShaderSystem>(), map_size_pixel );
  Factory::Shader::add_night_static( m_sys.find<Sys::Store::Type::ShaderSystem>(), map_size_pixel );
  Factory::Shader::add_red_vignette( m_sys.find<Sys::Store::Type::ShaderSystem>(),
                                     Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( m_reg ) );
  Factory::Shader::add_circular_distortion( m_sys.find<Sys::Store::Type::ShaderSystem>(),
                                            Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( m_reg ) );
  Factory::Shader::add_tunnel_vision( m_sys.find<Sys::Store::Type::ShaderSystem>(),
                                      Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( m_reg ) );
  Factory::Shader::add_dolly_zoom( m_sys.find<Sys::Store::Type::ShaderSystem>(), Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( m_reg ) );
  Factory::Shader::add_screen_shudder( m_sys.find<Sys::Store::Type::ShaderSystem>(),
                                       Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( m_reg ) );

  // Create the level contents - "reserved" spatial map prevents overlap
  auto &level_gen = m_sys.find<Sys::Store::Type::LevelGenerator>();
  level_gen.init( m_reserved_sm, m_item_sm );
  level_gen.build_scene_from_data( *m_scene_data );
  // world-tile lookup: built now, while only the tiles exist, so plant/multiblock placement can skip full registry scans
  m_world_tile_sm = Factory::Pathfinding::create_world_tile_grid( m_reg );
  level_gen.set_world_tile_sm( m_world_tile_sm );
  m_sys.find<Sys::Store::Type::PlantSystem>().init( m_reserved_sm, m_world_tile_sm, m_item_sm, m_hazard_sm );
  // one point of interest per map quadrant: one random quadrant is always the barber surgeon, a different random
  // quadrant is always the ruin, and the remaining two are crypts
  const auto poi_positions = m_scene_data->poi_positions_by_quadrant();
  const auto barber_quadrant = static_cast<std::size_t>( Cmp::RandomInt( 0, 3 ).gen() );
  // offset of 1..3 guarantees a different quadrant, uniformly chosen among the other three
  const auto ruin_quadrant = ( barber_quadrant + static_cast<std::size_t>( Cmp::RandomInt( 1, 3 ).gen() ) ) % poi_positions.size();
  std::vector<sf::Vector2f> crypt_positions;
  std::optional<sf::Vector2f> healing_pos;
  std::optional<sf::Vector2f> ruin_pos;
  for ( std::size_t quadrant = 0; quadrant < poi_positions.size(); ++quadrant )
  {
    const auto &pos = poi_positions.at( quadrant );
    if ( quadrant == barber_quadrant ) { healing_pos = pos; }
    else if ( quadrant == ruin_quadrant ) { ruin_pos = pos; }
    else { crypt_positions.push_back( pos ); }
  }
  SPDLOG_INFO( "Graveyard POIs: {} crypts, barber surgeon: {}, ruin: {}", crypt_positions.size(), healing_pos.has_value(), ruin_pos.has_value() );

  // the round map leaves less room near the edge: warn if a marker has been moved where its building no longer fits
  std::vector<std::pair<sf::Vector2f, std::string_view>> poi_footprints;
  for ( const auto &pos : crypt_positions )
  {
    poi_footprints.emplace_back( pos, "sprite.graveyard.crypt.closed" );
  }
  if ( healing_pos ) { poi_footprints.emplace_back( *healing_pos, "sprite.graveyard.building.healingspring" ); }
  if ( ruin_pos ) { poi_footprints.emplace_back( *ruin_pos, "sprite.graveyard.ruin" ); }
  for ( const auto &[pos, sprite_type] : poi_footprints )
  {
    const auto &ss = Sys::SpriteStore::instance().get( Sys::SpriteKey( std::string( sprite_type ) ) );
    if ( not m_scene_data->is_open_footprint( pos, sf::Vector2u( ss.get_grid_size() ) ) )
    {
      SPDLOG_ERROR( "{} at 'poi' marker ({}, {}) overlaps non-open levelgen tiles", sprite_type, pos.x, pos.y );
    }
  }
  level_gen.gen_graveyard_exterior_multiblocks( *m_scene_data, crypt_positions, healing_pos, ruin_pos );
  level_gen.gen_loot_containers( map_size_grid );
  level_gen.gen_npc_containers( *m_scene_data );
  level_gen.gen_random_plants( *m_scene_data );

  const auto &init_chance = Sys::PersistSystem::get<Cmp::Persist::GraveyardProcGenInitChance>( m_reg );
  level_gen.add_graveyard_exterior_obstacles( init_chance.get_value() );

  auto &cellauto_parser = m_sys.find<Sys::Store::Type::CellAutomataSystem>();
  cellauto_parser.iterate( level_gen.get_obstacle_sm(), level_gen.get_reserved_sm(), level_gen.get_claims_sm() );

  level_gen.decorate_graveyard_exterior_obstacles();
  // grass fills whatever ground is still free, so it goes last - before the claims protecting containers and obstacles are dropped
  level_gen.gen_grass( *m_scene_data );
  // obstacles, caps, loot containers and NPC spawn cells only needed protecting while the level was generated
  level_gen.discard_claims();

  // create navmeshes for pathfinding
  m_generic_npc_navmesh = Factory::Pathfinding::create_npc_navmesh( m_reg );
  m_ghost_navmesh = Factory::Pathfinding::create_ghost_navmesh( m_reg );
  m_player_navmesh = Factory::Pathfinding::create_player_navmesh( m_reg );
  m_open_navmesh = Factory::Pathfinding::create_open_navmesh( m_reg );
  m_loot_sm = Factory::Pathfinding::create_loot_grid( m_reg );

  // spatial index for RenderGameSystem's Cmp::Position z-order pass - kept up to date by a periodic
  // rebuild in do_update() rather than precise per-mutation sync, see kRenderPositionGridRebuildInterval
  m_render_position_sm = Factory::Pathfinding::create_render_position_grid( m_reg );

  reinit_system_spatial_maps();

  // create floor background
  Sprites::Containers::VertexFloor floortiles;
  floortiles.create( level_gen.get_void_sm(), m_scene_data );

  m_sys.find<Sys::Store::Type::RenderGameSystem>().init_world_view();

  // one hazard field per map quadrant, seeded on a random open tile in that quadrant: two sinkholes and two
  // corruption fields, shuffled across the quadrants. The reserved path cross keeps each field in its quadrant.
  std::array<std::vector<sf::Vector2f>, 4> open_by_quadrant;
  for ( const auto &pos : m_scene_data->levelgen_positions( m_scene_data->open_tile_id() ) )
  {
    open_by_quadrant.at( m_scene_data->quadrant_of( pos ) ).push_back( pos );
  }
  std::array<bool, 4> quadrant_is_sinkhole{ true, true, false, false };
  Cmp::RandomInt hazard_rng( 0, 1 );
  std::ranges::shuffle( quadrant_is_sinkhole, hazard_rng );
  std::vector<std::vector<sf::Vector2f>> sinkhole_sets;
  std::vector<std::vector<sf::Vector2f>> corruption_sets;
  for ( std::size_t quadrant = 0; quadrant < open_by_quadrant.size(); ++quadrant )
  {
    ( quadrant_is_sinkhole.at( quadrant ) ? sinkhole_sets : corruption_sets ).push_back( std::move( open_by_quadrant.at( quadrant ) ) );
  }

  for ( const auto &new_sinkhole_pos : m_sys.find<Sys::Store::Type::SinkHoleHazardSystem>().init_hazard_field( sinkhole_sets ) )
  {
    floortiles.remove( new_sinkhole_pos );
  }

  auto floor_entity = m_reg.create();
  m_reg.emplace<Sprites::Containers::VertexFloor>( floor_entity, floortiles );
  m_reg.emplace<Cmp::ZOrderValue>( floor_entity, -16.f );

  m_sys.find<Sys::Store::Type::CorruptionHazardSystem>().init_hazard_field( corruption_sets );
  m_sys.find<Sys::Store::Type::WormholeSystem>().spawn_wormhole( Sys::WormholeSystem::SpawnPhase::InitialSpawn );

  Factory::Particle::add_flame_for_world_items( m_reg, "graveyard.torch.particle.flame" );
}

void GraveyardScene::on_enter()
{
  SPDLOG_DEBUG( "Entering {}", get_name() );
  reinit_system_spatial_maps();

  auto &m_persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();

  m_sound_bank.stop_all();
  if ( m_sound_bank.get_music( "graveyard_music" ).getStatus() != sf::Sound::Status::Playing )
  {
    m_sound_bank.get_music( "graveyard_music" ).play();
    m_sound_bank.get_music( "graveyard_music" ).setLooping( true );
  }

  // prevent residual lerp movements from previous scene causing havoc in the new one
  Utils::Player::remove_lerp_cmp( m_reg );

  // the crypt door the player came out of locks behind them
  m_sys.find<Sys::Store::Type::CryptSystem>().lock_entered_crypt_doors();

  // Respawn player back in the graveyard: either at the last position when they left, or fallback to their start position
  const auto &player_pos = Utils::Player::get_position( m_reg );
  auto *player_last_graveyard_pos = Utils::Player::get_last_graveyard_pos( m_reg );
  if ( player_last_graveyard_pos != nullptr )
  {
    Sys::BaseSystem::get_systems_event_queue().trigger( Events::UpdatePlayerPositionEvent( player_last_graveyard_pos->position ) );
    SPDLOG_INFO( "Player re-entered graveyard at position ({}, {})", player_pos.position.x, player_pos.position.y );
  }
  else if ( m_just_spawned )
  {
    Sys::BaseSystem::get_systems_event_queue().trigger(
        Events::UpdatePlayerPositionEvent( sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( m_reg ) ) ) );
    SPDLOG_INFO( "Player entered graveyard at position ({}, {})", player_pos.position.x, player_pos.position.y );
    m_just_spawned = false;
  }

  // prevent the player from wandering off before the scene has loaded
  auto &player_dir = Utils::Player::get_direction( m_reg );
  player_dir = Cmp::Direction{ { 0.f, 0.f } };

  // check if the player inventory has a torch, if so light it up!
  Factory::Particle::add_flame_for_player_inventory_slot( m_reg );

  // Set the default footstep SFX for this scene
  m_reg.emplace_or_replace<Cmp::Player::Footstep>( Utils::Player::get_entity( m_reg ), Cmp::Player::Footstep::Type::GRASS );
}

void GraveyardScene::on_exit()
{
  SPDLOG_DEBUG( "Exiting {}", get_name() );
  // never leave the scene paused, e.g. with its sound effects still paused. Before the registry is cleared.
  m_sys.find<Sys::Store::Type::PauseOverlaySystem>().force_resume();
  m_reg.clear();
  m_sound_bank.get_music( "graveyard_music" ).stop();

  m_sys.find<Sys::Store::Type::FootstepSystem>().stop_footsteps_sound();

  for ( auto [floor_entt, floor_cmp] : m_reg.view<Sprites::Containers::VertexFloor>().each() )
  {
    floor_cmp.clear();
  }

  m_sound_bank.get_music( "graveyard_music" ).stop();
  m_sound_bank.get_effect( "skeleton_moving" ).stop();
}

void GraveyardScene::do_update( sf::Time dt )
{
  ZoneScoped;

  // the pause menu, the grimoire and pausing tooltips pause gameplay: the world is still drawn, but nothing in it advances
  const bool paused = Sys::OverlayArbitrationSystem::active_modal( m_reg ) != Sys::OverlayArbitrationSystem::Modal::NONE;
  const sf::Time world_dt = paused ? sf::Time::Zero : dt;

  // clamp the step: the first frame after a level loads has a dt covering the whole load (several seconds),
  // which would otherwise expire the depth display before it is ever seen
  Utils::Player::get_level_depth( m_reg ).display_timer += std::min( world_dt, sf::milliseconds( 100 ) );

  if ( not paused )
  {
    // keep m_render_position_sm up to date with static entities created/destroyed since it was last
    // built - see its doc comment for why this is a periodic rebuild rather than precise sync.
    // Repopulates the existing grid object in place (never reassigns m_render_position_sm) so systems
    // like InventorySystem that hold a weak_ptr to it from a one-time init() call don't go stale.
    m_render_position_grid_rebuild_timer += dt;
    if ( m_render_position_grid_rebuild_timer >= kRenderPositionGridRebuildInterval )
    {
      PROFILED( Factory::Pathfinding::populate_render_position_grid( m_reg, *m_render_position_sm ) );
      m_render_position_grid_rebuild_timer = sf::Time::Zero;
    }

    PROFILED( m_sys.find<Sys::Store::Type::AnimSystem>().update( dt ) );
    // before anything that checks Utils::Player::is_flying() this frame
    PROFILED( m_sys.find<Sys::Store::Type::FlyingSystem>().update( dt ) );
    {
      ZoneScopedN( "SinkHoleHazardSystem::update" );
      if ( auto pos = m_sys.find<Sys::Store::Type::SinkHoleHazardSystem>().update( dt ); pos != sf::Vector2f{ 0, 0 } )
      {
        for ( auto [floor_entt, floortiles] : m_reg.view<Sprites::Containers::VertexFloor>().each() )
        {
          floortiles.remove( pos );
        }
      }
    }

    PROFILED( m_sys.find<Sys::Store::Type::CorruptionHazardSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::BombSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::LootSystem>().check_loot_collision( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::LootSystem>().update_loot_container_particles() );
    PROFILED( m_sys.find<Sys::Store::Type::NpcSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::WispSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::WispSystem>().spawn_wisp() );
    PROFILED( m_sys.find<Sys::Store::Type::WatchmanSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::WormholeSystem>().check_player_wormhole_collision() );
    PROFILED( m_sys.find<Sys::Store::Type::TeleportSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::ActionSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::PlantSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::FootstepSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::DowsingGuideSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::GraveSystem>().update() );
    PROFILED( m_sys.find<Sys::Store::Type::CryptSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::AltarSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::HealingSpringSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::RuinSystem>().update( dt, m_scene_data->map_size().second ) );
    PROFILED( m_sys.find<Sys::Store::Type::PlayerSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::ShaderSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::LightningSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::ArrowSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::BurrSystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::InventorySystem>().update( dt ) );
    PROFILED( m_sys.find<Sys::Store::Type::CartHorseSystem>().update( dt ) );

    // embers arc over obstacles rather than colliding with them
    auto excludes = { std::string( "graveyard.plant.particle.flame" ), std::string( Cmp::Particle::NoteParticleSprite::kPlayerNotesTag ),
                      std::string( Cmp::Particle::EmberParticleSprite::kYewTag ) };
    m_sys.find<Sys::Store::Type::ParticleSystem>().check_collsion( excludes );
    PROFILED( m_sys.find<Sys::Store::Type::ParticleSystem>().update( dt ) );
  }

  PROFILED( m_sys.find<Sys::Store::Type::RenderGameSystem>().render_game( world_dt ) );
  // HUD, shop, tooltip, grimoire, pause and debug overlays, in one shared ImGui frame. Real dt even while paused, for ImGui.
  PROFILED( m_sys.find<Sys::Store::Type::OverlayArbitrationSystem>().update( dt, world_dt ) );
}

void GraveyardScene::reinit_system_spatial_maps()
{
  m_sys.find<Sys::Store::Type::NpcSystem>().init( m_generic_npc_navmesh, m_open_navmesh, m_ghost_navmesh, m_reserved_sm, m_loot_sm, m_item_sm );
  m_sys.find<Sys::Store::Type::WispSystem>().init( m_open_navmesh );
  m_sys.find<Sys::Store::Type::BombSystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_ghost_navmesh, m_reserved_sm, m_world_tile_sm,
                                                   m_hazard_sm );
  m_sys.find<Sys::Store::Type::ActionSystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_ghost_navmesh, m_reserved_sm );
  m_sys.find<Sys::Store::Type::PlantSystem>().init( m_reserved_sm, m_world_tile_sm, m_item_sm, m_hazard_sm, m_generic_npc_navmesh, m_player_navmesh,
                                                    m_ghost_navmesh, m_loot_sm );
  m_sys.find<Sys::Store::Type::LootSystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_ghost_navmesh, m_reserved_sm, m_loot_sm, m_item_sm );
  m_sys.find<Sys::Store::Type::ItemSystem>().init( m_reserved_sm, m_item_sm, m_generic_npc_navmesh, m_ghost_navmesh, m_loot_sm );
  m_sys.find<Sys::Store::Type::InventorySystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_reserved_sm, m_item_sm );
  m_sys.find<Sys::Store::Type::PlayerSystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_open_navmesh );
  m_sys.find<Sys::Store::Type::WormholeSystem>().init( m_generic_npc_navmesh, m_reserved_sm, m_world_tile_sm, m_item_sm, m_hazard_sm );
  m_sys.find<Sys::Store::Type::DebugOverlaySystem>().init( m_generic_npc_navmesh, m_reserved_sm, m_item_sm, m_hazard_sm );
  m_sys.find<Sys::Store::Type::WatchmanSystem>().init( m_scene_data->levelgen_positions( m_scene_data->path_tile_id() ), m_item_sm, m_hazard_sm );
  m_sys.find<Sys::Store::Type::SinkHoleHazardSystem>().init( m_reserved_sm, m_generic_npc_navmesh, m_world_tile_sm, m_loot_sm, m_item_sm,
                                                             m_hazard_sm );
  m_sys.find<Sys::Store::Type::CorruptionHazardSystem>().init( m_reserved_sm, m_generic_npc_navmesh, m_world_tile_sm, m_loot_sm, m_item_sm,
                                                               m_hazard_sm );
}

entt::registry &GraveyardScene::registry() { return m_reg; }

} // namespace Game::Scene