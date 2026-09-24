#include <Components/Altar/MultiBlock.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/Crypt/BuildingMultiBlock.hpp>
#include <Components/Crypt/BuildingSegment.hpp>
#include <Components/Crypt/InteriorMultiBlock.hpp>
#include <Components/Crypt/InteriorSegment.hpp>
#include <Components/Crypt/ObjectiveMultiBlock.hpp>
#include <Components/Crypt/ObjectiveSegment.hpp>
#include <Components/Crypt/RoomClosed.hpp>
#include <Components/Crypt/RoomEnd.hpp>
#include <Components/Crypt/RoomOpen.hpp>
#include <Components/Crypt/RoomStart.hpp>
#include <Components/Exit.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Grave/Segment.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LootContainer.hpp>
#include <Components/Moveable.hpp>
#include <Components/ObstacleCap.hpp>
#include <Components/Persistent/GraveNumMultiplier.hpp>
#include <Components/Persistent/MaxNumAltars.hpp>
#include <Components/Persistent/MaxNumCrypts.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Ruin/BuildingSegment.hpp>
#include <Components/Ruin/Cobweb.hpp>
#include <Components/Ruin/GateSegment.hpp>
#include <Components/Ruin/HexagramMultiBlock.hpp>
#include <Components/Ruin/HexagramSegment.hpp>
#include <Components/Ruin/RuneMarking.hpp>
#include <Components/Ruin/StairsBalustradeMultiBlock.hpp>
#include <Components/Ruin/StairsGateMultiBlock.hpp>
#include <Components/Ruin/StairsLowerMultiBlock.hpp>
#include <Components/Ruin/StairsUpperMultiBlock.hpp>
#include <Components/SpawnArea.hpp>
#include <Components/Spring/HealingSpringBuildingMultiBlock.hpp>
#include <Components/Spring/HealingSpringBuildingSegment.hpp>
#include <Components/Spring/HealingSpringMultiBlock.hpp>
#include <Components/Spring/HealingSpringSegment.hpp>
#include <Components/Wall.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Factory/CryptFactory.hpp>
#include <Factory/LootFactory.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/PathfindingFactory.hpp>
#include <Factory/PlantFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Factory/RuinFactory.hpp>
#include <Factory/WallFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SceneControl/SceneData.hpp>
#include <Sprites/SpriteMetaType.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/ProcGen/LevelGenerator.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>

#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <functional>
#include <optional>
#include <ranges>
#include <spdlog/fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace Game::Sys::ProcGen
{

LevelGenerator::LevelGenerator( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank ),
      m_obstacle_sm( std::make_unique<PathFinding::SpatialHashGrid>() ),
      m_void_sm( std::make_unique<PathFinding::SpatialHashGrid>() ),
      m_non_obstacle_sm( std::make_unique<PathFinding::SpatialHashGrid>() )
{
}

PathFinding::SpatialHashGrid &LevelGenerator::get_obstacle_sm() { return *m_obstacle_sm; }
PathFinding::SpatialHashGrid &LevelGenerator::get_void_sm() { return *m_void_sm; }
PathFinding::SpatialHashGrid &LevelGenerator::get_non_obstacle_sm() { return *m_non_obstacle_sm; }
PathFinding::SpatialHashGrid &LevelGenerator::get_reserved_sm() { return *m_reserved_sm; }

void LevelGenerator::cleanup_reserved()
{
  for ( auto entt : reg().view<Cmp::Obstacle, Cmp::Position>() )
  {
    m_reserved_sm->remove( entt, reg().get<Cmp::Position>( entt ) );
  }
  for ( auto entt : reg().view<Cmp::ObstacleCap, Cmp::Position>() )
  {
    m_reserved_sm->remove( entt, reg().get<Cmp::Position>( entt ) );
  }
  for ( auto entt : reg().view<Cmp::LootContainer, Cmp::Position>() )
  {
    m_reserved_sm->remove( entt, reg().get<Cmp::Position>( entt ) );
  }
}

void LevelGenerator::build_scene_from_data( const Scene::SceneData &scene_data )
{

  auto [map_size_grid, map_size_pixel] = scene_data.map_size();
  auto w = map_size_grid.x;

  // Walls
  const Sprites::SpriteSheet &wall_ms = m_sprite_factory.get_spritesheet_by_type( scene_data.wall_tileset().name );
  for ( const auto [i, tile] : std::views::enumerate( scene_data.wall_tilelayer() ) )
  {
    auto row = i / w; // increments every 'w' tiles
    auto col = i % w; // wraps back to zero every 'w' tiles
    sf::Vector2f new_pos( static_cast<float>( col ) * Constants::kGridSizePxF.x, static_cast<float>( row ) * Constants::kGridSizePxF.y );
    if ( tile >= scene_data.wall_tileset().first_gid )
    {
      // get the relative index using the offset from the json data
      auto entt = Factory::Wall::add_wall_entity( reg(), new_pos, wall_ms, tile - scene_data.wall_tileset().first_gid );
      m_reserved_sm->insert( entt, Cmp::Position( new_pos, Constants::kGridSizePxF ) );
    }
  }

  // Levelgen layer
  for ( const auto [i, tile] : std::views::enumerate( scene_data.levelgen_tilelayer() ) )
  {
    auto col = i % w; // wraps back to zero every 'w' tiles
    auto row = i / w; // increments every 'w' tiles
    sf::Vector2f new_pos( static_cast<float>( col ) * Constants::kGridSizePxF.x, static_cast<float>( row ) * Constants::kGridSizePxF.y );
    if ( tile == scene_data.void_tile_id() )
    {
      Cmp::Position new_pos_cmp( new_pos, Constants::kGridSizePxF );
      auto entt = reg().create();
      m_void_sm->insert( entt, new_pos_cmp );
    }
    else if ( tile == scene_data.open_tile_id() ) { Factory::Obstacle::create_world_pos( reg(), new_pos ); }
    else if ( tile == scene_data.spawn_tile_id() )
    {
      auto entity = Factory::Obstacle::create_world_pos( reg(), new_pos );
      Factory::Player::add_spawn_area( reg(), entity, m_sprite_factory, new_pos.y - 16.0f );
      m_reserved_sm->insert( entity, Cmp::Position( new_pos, Constants::kGridSizePxF ) );
    }
    else if ( tile == scene_data.exit_tile_id() )
    {
      auto entt = Factory::Crypt::create_crypt_exit( reg(), new_pos );
      m_reserved_sm->insert( entt, Cmp::Position( new_pos, Constants::kGridSizePxF ) );
    }
    else if ( tile == scene_data.reserved_tile_id() )
    {
      auto entt = Factory::Wall::add_reservedposition( reg(), new_pos );
      m_reserved_sm->insert( entt, Cmp::Position( new_pos, Constants::kGridSizePxF ) );
    }
  }

  // Solid Layer
  for ( const auto &solid : scene_data.solid_objectlayer() )
  {
    Factory::Wall::add_solid_player( reg(), solid, *m_reserved_sm );
    Factory::Wall::add_solid_npc( reg(), solid, *m_reserved_sm );
    Factory::Wall::add_no_move_dest( reg(), solid, *m_reserved_sm );
  }

  // clang-format off
  // Multiblock layer
  using MultiblockFactoryFn = std::function<entt::entity( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet & )>;
  const std::unordered_map<std::string, MultiblockFactoryFn> kMultiblockFactories{
      { "sprite.graveyard.healingspring", [this]( entt::registry &r, sf::Vector2f p, const Sprites::SpriteSheet &ss )
        { return Factory::Multiblock::add_multiblock_with_segments<Cmp::HealingSpringMultiBlock, Cmp::HealingSpringSegment>( r, p, ss, 0, 0, m_reserved_sm.get() ).first; } },
      { "sprite.crypt.objective.closed",[this]( entt::registry &r, sf::Vector2f p, const Sprites::SpriteSheet &ss )
        { return Factory::Multiblock::add_multiblock_with_segments<Cmp::Crypt::ObjectiveMultiBlock, Cmp::Crypt::ObjectiveSegment>( r, p, ss, 0, 0, m_reserved_sm.get() ).first; } },
      { "sprite.crypt.altar.inactive", [this]( entt::registry &r, sf::Vector2f p, const Sprites::SpriteSheet &ss )
        { return Factory::Multiblock::add_multiblock_with_segments<Cmp::Altar::MultiBlock, Cmp::Altar::Segment>( r, p, ss, 0, 0, m_reserved_sm.get() ).first; } },
      { "sprite.ruin.stairs.up", [this]( entt::registry &r, sf::Vector2f p, const Sprites::SpriteSheet &ss )
        { return Factory::Multiblock::add_multiblock_with_segments<Cmp::Ruin::StairsLowerMultiBlock, Cmp::Ruin::StairsSegment>( r, p, ss, 0, 0, m_reserved_sm.get() ).first; } },
      { "sprite.ruin.stairs.down", [this]( entt::registry &r, sf::Vector2f p, const Sprites::SpriteSheet &ss )
        { return Factory::Multiblock::add_multiblock_with_segments<Cmp::Ruin::StairsUpperMultiBlock, Cmp::Ruin::StairsSegment>( r, p, ss, 0, 0, m_reserved_sm.get() ).first; } },
      { "sprite.ruin.stairs.balustrade", [this]( entt::registry &r, sf::Vector2f p, const Sprites::SpriteSheet &ss )
        { return Factory::Multiblock::add_multiblock_with_segments<Cmp::Ruin::StairsBalustradeMultiBlock, Cmp::Ruin::StairsSegment>( r, p, ss, 0, 0, m_reserved_sm.get() ).first; } },
      { "sprite.ruin.stairs.gate", [this]( entt::registry &r, sf::Vector2f p, const Sprites::SpriteSheet &ss )
        { return Factory::Multiblock::add_multiblock_with_segments<Cmp::Ruin::StairsGateMultiBlock, Cmp::Ruin::GateSegment>( r, p, ss, 0, 0, m_reserved_sm.get() ).first; } },
      { "sprite.ruin.hex", [this]( entt::registry &r, sf::Vector2f p, const Sprites::SpriteSheet &ss )
        { return Factory::Multiblock::add_multiblock_with_segments<Cmp::Ruin::HexagramMultiBlock, Cmp::Ruin::HexagramSegment>( r, p, ss, 0, 0, m_reserved_sm.get() ).first; } },
  };
  // clang-format on

  for ( const auto &[ms_type, pos] : scene_data.multiblock_objectlayer() )
  {
    // allow "npc." markers in the multiblock layer
    if ( ms_type.contains( "npc." ) )
    {
      auto npc_entt = reg().create();
      reg().emplace_or_replace<Cmp::Position>( npc_entt, pos, Constants::kGridSizePxF );
      Factory::Npc::create_npc( reg(), npc_entt, ms_type, m_reserved_sm );
      continue;
    }

    // Item markers share this layer but are named by item id (e.g. "item.axe"), which isn't
    // itself a valid sprite key - resolve the item's actual sprite type before lookup.
    const Sprites::SpriteMetaType sprite_lookup_type = ms_type.contains( "item." ) ? Sys::ItemStore::instance().get_item( ms_type ).sprite_type
                                                                                   : ms_type;
    const auto &ms = m_sprite_factory.get_spritesheet_by_type( sprite_lookup_type );

    if ( auto it = kMultiblockFactories.find( ms_type ); it != kMultiblockFactories.end() )
    {
      auto mb_entt = it->second( reg(), pos, ms );
      m_reserved_sm->insert( mb_entt, Cmp::Position( pos, Constants::kGridSizePxF ) );
    }
    else if ( ms_type.contains( "item.plant" ) ) { gen_plant( ms_type, pos ); }
    else if ( ms_type.contains( "item." ) )
    {
      // prevent infinite respawns in the RuinSceneUpperFloor
      auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
      if ( inventory_type == "item.witchesjar" and ms.get_sprite_type().contains( "witchesjar" ) ) continue;

      // make sure we mark the *world* entt as reserved
      auto world_pos_entt = Utils::get_world_pos_entt( reg(), Cmp::Position( pos, ms.get_sprite_size() ) );
      if ( world_pos_entt != entt::null )
      {
        m_reserved_sm->insert( world_pos_entt, Cmp::Position( pos, ms.get_sprite_size() ) );
        get_systems_event_queue().trigger( Events::CreateItemEvent( Cmp::Position( pos, ms.get_sprite_size() ), ms_type ) );
      }
    }
  }
}

void LevelGenerator::try_place_obstacle( entt::entity entity, const Cmp::Position &pos_cmp, float init_chance, bool pass_navmesh_to_factory )
{
  if ( Cmp::RandomFloat{ 0.f, 1.f }.gen() >= init_chance ) return;

  bool placed;
  if ( pass_navmesh_to_factory ) { placed = Factory::Obstacle::add_obstacle( reg(), entity, m_reserved_sm.get() ); }
  else
  {
    Factory::Obstacle::add_obstacle( reg(), entity );
    placed = true;
  }

  if ( placed )
  {
    m_obstacle_sm->insert( entity, pos_cmp );
    m_reserved_sm->insert( entity, pos_cmp );
  }
}

void LevelGenerator::add_graveyard_exterior_obstacles( float init_chance )
{
  auto position_view = reg().view<Cmp::Position>( entt::exclude<Cmp::Player::Character> );
  for ( auto [entity, pos_cmp] : position_view.each() )
  {
    if ( not m_reserved_sm->at( pos_cmp ).empty() ) continue;
    try_place_obstacle( entity, pos_cmp, init_chance, /*pass_navmesh_to_factory=*/true );
  }
}

void LevelGenerator::decorate_obstacles( const Sprites::SpriteSheet &ss_main, const Sprites::SpriteSheet &ss_cap,
                                         const std::function<std::size_t()> &pick_index, float cap_y_offset, bool moveable )
{
  // Note: entities with Cmp::Obstacle here are never themselves reserved (try_place_obstacle only
  // inserts them into m_reserved_sm to block *other* candidates from being picked, which is exactly
  // why every one of them must still be decorated here).
  auto obstacle_view = reg().view<Cmp::Obstacle, Cmp::Position>( entt::exclude<Cmp::Player::Character> );
  for ( auto [obstacle_entt, obstacle_cmp, obstacle_pos_cmp] : obstacle_view.each() )
  {
    auto uuid = Cmp::UUID::generate();
    auto idx = pick_index();

    Factory::Obstacle::decorate_obstacle( reg(), obstacle_entt, obstacle_pos_cmp, ss_main, idx );
    reg().emplace_or_replace<Cmp::UUID>( obstacle_entt, uuid );
    if ( moveable ) reg().emplace_or_replace<Cmp::Moveable>( obstacle_entt );

    auto cap_entt = reg().create();
    Cmp::Position cap_position( { obstacle_pos_cmp.x(), obstacle_pos_cmp.y() - obstacle_pos_cmp.size.y }, obstacle_pos_cmp.size );
    reg().emplace_or_replace<Cmp::Position>( cap_entt, cap_position );
    Factory::Obstacle::decorate_obstacle( reg(), cap_entt, cap_position, ss_cap, idx, obstacle_pos_cmp.y() + cap_y_offset, false );
    m_reserved_sm->insert( cap_entt, cap_position );
    reg().emplace_or_replace<Cmp::UUID>( cap_entt, uuid );
    Factory::Obstacle::add_obstacle_cap( reg(), cap_entt );
  }
}

void LevelGenerator::decorate_graveyard_exterior_obstacles()
{
  const Sprites::SpriteSheet &ss_main = m_sprite_factory.get_spritesheet_by_type( "sprite.graveyard.wall.int.main" );
  const Sprites::SpriteSheet &ss_cap = m_sprite_factory.get_spritesheet_by_type( "sprite.graveyard.wall.int.cap" );

  decorate_obstacles( ss_main, ss_cap,
                      [this]() { return m_sprite_factory.get_random_type_and_texture_index( { "sprite.graveyard.wall.int.main" } ).second; },
                      /*cap_y_offset=*/1.f, /*moveable=*/false );
}

void LevelGenerator::add_ruin_interior_obstacles( float init_chance )
{
  auto position_view = reg().view<Cmp::Position>( entt::exclude<Cmp::Player::Character, Cmp::Exit> );
  for ( auto [entity, pos_cmp] : position_view.each() )
  {
    if ( m_reserved_sm->at( pos_cmp ).empty() ) { try_place_obstacle( entity, pos_cmp, init_chance, /*pass_navmesh_to_factory=*/false ); }
  }
}

void LevelGenerator::decorate_ruin_interior_obstacles()
{
  const Sprites::SpriteSheet &ss_main = m_sprite_factory.get_spritesheet_by_type( "sprite.ruin.wall.int.main" );
  const Sprites::SpriteSheet &ss_cap = m_sprite_factory.get_spritesheet_by_type( "sprite.ruin.wall.int.cap" );

  decorate_obstacles( ss_main, ss_cap, []() -> std::size_t { return 0; },
                      /*cap_y_offset=*/0.f, /*moveable=*/true );
}

void LevelGenerator::add_ruin_rune_markers()
{
  constexpr int kRuneMarkerCount = 5;
  constexpr int kMaxAttempts = 1000;

  int placed = 0;
  int attempts = 0;
  while ( placed < kRuneMarkerCount && attempts < kMaxAttempts )
  {
    attempts++;
    auto [rnd_entt, rnd_pos] = Utils::Rnd::get_random_position( reg(), {}, Utils::Rnd::ExcludePack<Cmp::Player::Character>{} );
    // A random pick can land on an already-reserved position - retry with a new pick rather than
    // giving up on this rune marker, otherwise unlucky rolls silently place fewer than intended.
    if ( not m_reserved_sm->at( rnd_pos ).empty() ) continue;
    m_reserved_sm->insert( rnd_entt, rnd_pos );
    auto [_, idx] = m_sprite_factory.get_random_type_and_texture_index( { "sprite.ruin.runemarking.inactive" } );
    float zorder = m_sprite_factory.get_spritesheet_by_type( "sprite.ruin.runemarking.inactive" ).get_zorder( 0 );
    auto rune_entt = Factory::Ruin::create_rune_marker( reg(), rnd_pos, zorder, idx, *m_reserved_sm );

    SPDLOG_DEBUG( "Added rune to {},{}", rnd_pos.x(), rnd_pos.y() );

    m_non_obstacle_sm->insert( rune_entt, rnd_pos );
    placed++;
  }

  if ( placed < kRuneMarkerCount ) { SPDLOG_ERROR( "Only placed {}/{} rune markers after {} attempts.", placed, kRuneMarkerCount, attempts ); }
}

void LevelGenerator::add_lowerfloor_cobwebs( int num_cobwebs, sf::FloatRect scene_dimensions )
{
  auto has_collision = [&]( const Cmp::RectBounds &pos )
  {
    if ( Utils::Collision::any_intersects<Cmp::Ruin::StairsLowerMultiBlock>( reg(), pos ) ) { return true; }
    if ( Utils::Collision::any_intersects<Cmp::Ruin::Cobweb>( reg(), pos ) ) { return true; }
    if ( Utils::Collision::any_intersects<Cmp::Exit>( reg(), pos ) ) { return true; }

    // ensure is inside scene
    if ( not Cmp::RectBounds::scaled( pos.position(), pos.size(), 1.5f ).findIntersection( scene_dimensions ) ) { return true; }
    return false;
  };

  constexpr auto gridsize = Constants::kGridSizePxF;
  constexpr int kMaxAttempts = 1000;
  int placed = 0;
  int attempts = 0;
  // Retry on a wasted pick (already reserved or colliding) rather than giving up on that cobweb,
  // otherwise unlucky rolls silently place fewer than num_cobwebs.
  while ( placed < num_cobwebs && attempts < kMaxAttempts )
  {
    attempts++;
    auto [rnd_entt, rnd_pos] = Utils::Rnd::get_random_position( reg(), {}, Utils::Rnd::ExcludePack<Cmp::Player::Character>{} );
    if ( rnd_entt == entt::null ) continue;
    if ( not m_reserved_sm->at( rnd_pos ).empty() ) continue;

    if ( has_collision( Cmp::RectBounds::scaled( { rnd_pos.position }, gridsize, 1 ) ) ) continue;
    auto [ms, idx] = m_sprite_factory.get_random_type_and_texture_index( { "sprite.ruin.cobweb" } );
    Factory::Ruin::create_cobweb( reg(), rnd_entt, rnd_pos.position, m_sprite_factory.get_spritesheet_by_type( ms ), idx, *m_reserved_sm );
    m_non_obstacle_sm->insert( rnd_entt, rnd_pos );
    placed++;
  }
}

void LevelGenerator::gen_graveyard_exterior_multiblocks()
{
  auto grave_num_multiplier = Sys::PersistSystem::get<Cmp::Persist::GraveNumMultiplier>( reg() );
  auto max_num_altars = Sys::PersistSystem::get<Cmp::Persist::MaxNumAltars>( reg() );
  auto max_num_crypts = Sys::PersistSystem::get<Cmp::Persist::MaxNumCrypts>( reg() );
  std::size_t max_number_healing_springs = 1;
  std::size_t max_number_ruins = 1;

  // shared spawn-location lookup for every multiblock placed below
  auto find_spawn_pos = [&]( const Sprites::SpriteSheet &ms ) -> std::optional<Cmp::Position>
  {
    auto [random_entity, random_origin_position] = find_spawn_location( ms, 0 );
    if ( random_entity == entt::null )
    {
      SPDLOG_ERROR( "Failed to find valid spawn position for {}.", ms.get_sprite_type() );
      return std::nullopt;
    }
    return random_origin_position;
  };

  // GRAVES
  auto grave_meta_types = m_sprite_factory.get_all_sprite_types_by_pattern( R"(graves\.\w+\.closed$)" );
  if ( grave_meta_types.size() < 2 ) { SPDLOG_WARN( "No GRAVE spritesheets found in SpriteFactory" ); }
  else
  {
    SPDLOG_DEBUG( "Found {}, {}", grave_meta_types[0], grave_meta_types[1] );
    auto max_num_graves = static_cast<size_t>( max_num_altars.get_value() * grave_num_multiplier.get_value() );
    for ( std::size_t i = 0; i < max_num_graves; ++i )
    {
      auto [sprite_metatype, index] = m_sprite_factory.get_random_type_and_texture_index( grave_meta_types );
      SPDLOG_DEBUG( "Selected {}, {}", sprite_metatype, index );
      const auto &spritesheet = m_sprite_factory.get_spritesheet_by_type( sprite_metatype );
      if ( auto pos = find_spawn_pos( spritesheet ) )
      {
        auto [mb_entt, _] = Factory::Multiblock::add_multiblock_with_segments<Cmp::Grave::MultiBlock, Cmp::Grave::Segment>(
            reg(), pos->position, spritesheet, index, 0, m_reserved_sm.get() );
        m_reserved_sm->insert( mb_entt, pos.value() );
      }
    }
  }

  // ALTARS
  const auto &altar_spritesheet = m_sprite_factory.get_spritesheet_by_type( "sprite.graveyard.altar.inactive" );
  spawn_multiblocks<Cmp::Altar::MultiBlock, Cmp::Altar::Segment>( static_cast<std::size_t>( max_num_altars.get_value() ), altar_spritesheet );

  // CRYPTS - note: we use keys from altars to open crypts so the number should be equal
  const auto &crypt_spritesheet = m_sprite_factory.get_spritesheet_by_type( "sprite.graveyard.crypt.closed" );
  spawn_multiblocks<Cmp::Crypt::BuildingMultiBlock, Cmp::Crypt::BuildingSegment>( static_cast<std::size_t>( max_num_crypts.get_value() ),
                                                                                  crypt_spritesheet, /*log=*/true );

  const auto &healingspring_spritesheet = m_sprite_factory.get_spritesheet_by_type( "sprite.graveyard.building.healingspring" );
  spawn_multiblocks<Cmp::HealingSpringBuildingMultiBlock, Cmp::HealingSpringBuildingSegment>( max_number_healing_springs, healingspring_spritesheet,
                                                                                              /*log=*/true );

  const auto &ruin_spritesheet = m_sprite_factory.get_spritesheet_by_type( "sprite.graveyard.ruin" );
  spawn_multiblocks<Cmp::Ruin::BuildingMultiBlock, Cmp::Ruin::BuildingSegment>( max_number_ruins, ruin_spritesheet, /*log=*/true );
}

template <typename MULTIBLOCK, typename MBSEGMENT>
void LevelGenerator::spawn_multiblocks( std::size_t count, const Sprites::SpriteSheet &ss, bool log )
{
  for ( std::size_t i = 0; i < count; ++i )
  {
    auto [random_entity, random_origin_position] = find_spawn_location( ss, 0 );
    if ( random_entity == entt::null )
    {
      SPDLOG_ERROR( "Failed to find valid spawn position for {}.", ss.get_sprite_type() );
      continue;
    }

    auto [mb_entt, _] = Factory::Multiblock::add_multiblock_with_segments<MULTIBLOCK, MBSEGMENT>( reg(), random_origin_position.position, ss, 0, 0,
                                                                                                  m_reserved_sm.get() );
    m_reserved_sm->insert( mb_entt, random_origin_position );
    if ( log ) { SPDLOG_INFO( "Added {} to {},{}", ss.get_sprite_type(), random_origin_position.position.x, random_origin_position.position.y ); }
  }
}

std::pair<entt::entity, Cmp::Position> LevelGenerator::find_spawn_location( const Sprites::SpriteSheet &ms, unsigned long seed )
{
  constexpr int kMaxAttempts = 1000;
  int attempts = 0;
  unsigned long current_seed = seed;

  while ( attempts < kMaxAttempts )
  {
    auto [random_entity, random_pos] = Utils::Rnd::get_random_position( reg(), Utils::Rnd::IncludePack<>{},
                                                                        Utils::Rnd::ExcludePack<Cmp::Wall, Cmp::Player::Character>{}, current_seed );

    auto lo_sprite_size = m_sprite_factory.get_sprite_size_by_type( ms.get_sprite_type() );
    auto new_lo_hitbox = Cmp::RectBounds::scaled( random_pos.position, lo_sprite_size, 1.f );

    // Check collisions with walls, graves, shrines, and anything else already claiming this position
    auto is_valid = [&]() -> bool
    {
      using Utils::Collision::any_intersects;
      return not( any_intersects<Cmp::Wall>( reg(), new_lo_hitbox ) || any_intersects<Cmp::Grave::Segment>( reg(), new_lo_hitbox ) ||
                  any_intersects<Cmp::Altar::Segment>( reg(), new_lo_hitbox ) ||
                  any_intersects<Cmp::Crypt::BuildingSegment>( reg(), new_lo_hitbox ) ||
                  any_intersects<Cmp::HealingSpringBuildingSegment>( reg(), new_lo_hitbox ) ||
                  any_intersects<Cmp::Ruin::BuildingSegment>( reg(), new_lo_hitbox ) ||
                  any_intersects<Cmp::Crypt::ObjectiveSegment>( reg(), new_lo_hitbox ) ||
                  not m_reserved_sm->query_rect( new_lo_hitbox.getBounds() ).empty() || any_intersects<Cmp::SpawnArea>( reg(), new_lo_hitbox ) ||
                  any_intersects<Cmp::Player::Character>( reg(), new_lo_hitbox ) );
    };

    if ( is_valid() )
    {
      if ( current_seed != seed && seed > 0 )
      {
        SPDLOG_WARN( "Large Obstacle spawn: original seed {} was invalid, used seed {} instead (attempt {})", seed, current_seed, attempts + 1 );
      }
      auto new_entt = reg().create();
      return { new_entt, reg().emplace_or_replace<Cmp::Position>( new_entt, random_pos.position, random_pos.size ) };
    }

    attempts++;
    // Increment seed for next attempt (works for both seeded and non-seeded cases)
    if ( seed > 0 ) { current_seed++; }
  }

  SPDLOG_ERROR( "Failed to find valid large obstacle spawn location after {} attempts (original seed: {})", kMaxAttempts, seed );
  return { entt::null, Cmp::Position{ { 0.f, 0.f }, { 0.f, 0.f } } };
}

bool LevelGenerator::gen_plant( const std::string &plant_type, sf::Vector2f pos )
{
  const auto &plant_ss = m_sprite_factory.get_spritesheet_by_type( "sprite." + plant_type );

  // Plants can span more than one grid row (e.g. 1x2), so every cell in the footprint - not
  // just the origin - must be a real, unreserved world tile. Otherwise a plant can overlap a
  // reserved tile it doesn't share an entity with, e.g. the player spawn.
  auto plant_grid_size = plant_ss.get_grid_size();
  bool footprint_clear = true;
  for ( int gy = 0; gy < plant_grid_size.y && footprint_clear; ++gy )
  {
    for ( int gx = 0; gx < plant_grid_size.x; ++gx )
    {
      sf::Vector2f pos_offset( static_cast<float>( gx ) * Constants::kGridSizePxF.x, static_cast<float>( gy ) * Constants::kGridSizePxF.y );
      Cmp::Position cell_pos( pos + pos_offset, Constants::kGridSizePxF );
      if ( Utils::get_world_pos_entt( reg(), cell_pos ) == entt::null || not m_reserved_sm->at( cell_pos ).empty() )
      {
        footprint_clear = false;
        break;
      }
    }
  }

  if ( footprint_clear )
  {
    // now create the plant at a new entt
    auto [mb_entt, _] = Factory::Multiblock::add_multiblock_with_segments<Cmp::PlantMultiBlock, Cmp::PlantSegment>( reg(), pos, plant_ss, 0, 0,
                                                                                                                    m_reserved_sm.get() );

    // Add the worlditem now so we don't have to look it up later when digging up the plant
    reg().emplace_or_replace<Cmp::WorldItem>( mb_entt, Sys::ItemStore::instance().get_item( plant_type ) );

    // Protect every cell covered by this plant: segment entities carry the UUID and
    // are at each tile position. Insert them so subsequent placement calls see them
    // as reserved via the O(1) navmesh check.
    auto *mb_uuid = reg().try_get<Cmp::UUID>( mb_entt );
    if ( mb_uuid )
    {
      for ( auto [seg_entt, seg_cmp, seg_pos, seg_uuid] : reg().view<Cmp::PlantSegment, Cmp::Position, Cmp::UUID>().each() )
      {
        if ( seg_uuid == *mb_uuid ) m_reserved_sm->insert( seg_entt, seg_pos );
      }
    }
    return true;
  }
  return false;
}

std::vector<entt::entity> LevelGenerator::gen_loot_containers( Sprites::SpriteFactory &sprite_factory, sf::Vector2u map_grid_size )
{
  std::vector<entt::entity> assigned_entts;

  auto num_loot_containers = map_grid_size.x * map_grid_size.y / 120; // one loot container per N grid squares

  for ( std::size_t i = 0; i < num_loot_containers; ++i )
  {
    auto [random_entity,
          random_origin_position] = Utils::Rnd::get_random_position( reg(), {}, Utils::Rnd::ExcludePack<Cmp::Player::Character, Cmp::Obstacle>{}, 0 );

    if ( m_reserved_sm->at( random_origin_position ).empty() )
    {
      float zorder = sprite_factory.get_sprite_size_by_type( "sprite.graveyard.pots" ).y;

      Cmp::RandomInt pot_picker( 0, 2 );
      Factory::Loot::create_loot_container( reg(), random_entity, random_origin_position, "sprite.graveyard.pots", pot_picker.gen(), zorder );
      assigned_entts.push_back( random_entity );
      m_reserved_sm->insert( random_entity, random_origin_position );
    }
  }

  return assigned_entts;
}

std::vector<entt::entity> LevelGenerator::gen_npc_containers( Sprites::SpriteFactory &sprite_factory, sf::Vector2u map_grid_size )
{
  std::vector<entt::entity> assigned_entts;

  auto num_npc_containers = map_grid_size.x * map_grid_size.y / 120; // one NPC container per N grid squares

  for ( std::size_t i = 0; i < num_npc_containers; ++i )
  {
    auto [random_entity,
          random_origin_position] = Utils::Rnd::get_random_position( reg(), {}, Utils::Rnd::ExcludePack<Cmp::Player::Character, Cmp::Obstacle>{}, 0 );

    if ( m_reserved_sm->at( random_origin_position ).empty() )
    {
      // pick a random loot container type and texture index
      // clang-format off
      auto [npc_type, rand_npc_tex_idx] =
        sprite_factory.get_random_type_and_texture_index( {
          "sprite.graveyard.bones"
        } );
      // clang-format on

      Factory::Npc::create_npc_container( reg(), random_entity, random_origin_position, npc_type, rand_npc_tex_idx, 0.f );
      assigned_entts.push_back( random_entity );
      m_reserved_sm->insert( random_entity, random_origin_position );
    }
  }
  return assigned_entts;
}

std::vector<entt::entity> LevelGenerator::gen_random_plants()
{
  std::vector<entt::entity> assigned_entts;

  constexpr int kMinCountPerPlantType = 10;

  // Find all plant sprites but exclude any ".drop" sprites. The map value will track count of each plant type added.
  std::unordered_map<Sprites::SpriteMetaType, int> plant_tracker;
  plant_tracker = m_sprite_factory.get_all_sprite_types_by_pattern( R"(sprite\.item\.plant\.(?!.*\.drop$).*)" ) |
                  std::views::transform( []( const auto &type ) { return std::pair{ type, 0 }; } ) |
                  std::ranges::to<std::unordered_map<Sprites::SpriteMetaType, int>>();

  // helpers
  const auto below_quota = [&]( const auto &kv ) { return kv.second < kMinCountPerPlantType; };
  const auto quota_reached = [&] { return std::ranges::none_of( plant_tracker, below_quota ); };

  // guard against endless failure (e.g. no free tiles left)
  std::size_t attempts = 0;
  const std::size_t max_attempts = plant_tracker.size() * kMinCountPerPlantType * 10;

  while ( not quota_reached() and attempts++ < max_attempts )
  {
    auto [random_entity, random_pos] = Utils::Rnd::get_random_position( reg(), {}, Utils::Rnd::ExcludePack<Cmp::Player::Character, Cmp::Obstacle>{},
                                                                        0 );

    // Build a list of plant type that are still short of the quota and pick one by random
    std::vector<decltype( plant_tracker )::iterator> candidates;
    for ( auto it = plant_tracker.begin(); it != plant_tracker.end(); ++it )
    {
      if ( below_quota( *it ) ) candidates.push_back( it );
    }
    auto selected_plant_iter = candidates.at( Cmp::RandomInt( 0, static_cast<int>( candidates.size() - 1 ) ).gen() );

    // gen_plant() re-adds the "sprite." prefix itself (to match the bare "item.plant.*" markers used elsewhere), so strip it back off here.
    const std::string kSpritePrefix = "sprite.";
    auto chosen_plant_item_type = selected_plant_iter->first.substr( kSpritePrefix.size() );

    // Create the plant in the game area and increment the tracker counter
    if ( gen_plant( chosen_plant_item_type, random_pos.position ) )
    {
      assigned_entts.push_back( random_entity );
      ++selected_plant_iter->second;
    }
  }

  if ( not quota_reached() )
  {
    SPDLOG_WARN( "gen_random_plants: hit attempt cap ({}) before every plant type reached its minimum count", max_attempts );
  }

  SPDLOG_INFO( "plant_tracker: {}", plant_tracker );
  return assigned_entts;
}

void LevelGenerator::init( const PathFinding::SpatialHashGridSharedPtr &reserved_sm )
{
  m_obstacle_sm->clear();
  m_void_sm->clear();
  m_non_obstacle_sm->clear();

  m_reserved_sm = reserved_sm;
  m_reserved_sm->clear();
}

} // namespace Game::Sys::ProcGen