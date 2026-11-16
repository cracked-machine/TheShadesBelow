#include <Components/Altar/MultiBlock.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/Altar/ArtefactCount.hpp>
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
#include <Components/Grave/Consequence.hpp>
#include <Components/Grave/ExitSegment.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Grave/Segment.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LootContainer.hpp>
#include <Components/Moveable.hpp>
#include <Components/Npc/Footprint.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/ObstacleCap.hpp>
#include <Components/Persistent/ArtefactMinChance.hpp>
#include <Components/Persistent/GraveCountMargin.hpp>
#include <Components/Persistent/GraveNumMultiplier.hpp>
#include <Components/Persistent/GraveOddsArtefact.hpp>
#include <Components/Persistent/GraveOddsBombTrap.hpp>
#include <Components/Persistent/GraveOddsCurseTablet.hpp>
#include <Components/Persistent/GraveOddsNpcTrap.hpp>
#include <Components/Persistent/GraveOddsTalisman.hpp>
#include <Components/Persistent/GraveOddsTorch.hpp>
#include <Components/Persistent/GraveTrailNominalHopCount.hpp>
#include <Components/Persistent/MaxLevelDepth.hpp>
#include <Components/Persistent/HealingBuildingOverride.hpp>
#include <Components/Persistent/MinNumRuins.hpp>
#include <Components/Persistent/NumAltars.hpp>
#include <Components/Player/Cadaver.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/UUID.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Ruin/BuildingMultiBlock.hpp>
#include <Components/Ruin/BuildingSegment.hpp>
#include <Components/Ruin/Cobweb.hpp>
#include <Components/Ruin/GateSegment.hpp>
#include <Components/Ruin/HexagramMultiBlock.hpp>
#include <Components/Ruin/HexagramSegment.hpp>
#include <Components/Ruin/RuneMarking.hpp>
#include <Components/Ruin/StairsBalustradeMultiBlock.hpp>
#include <Components/Ruin/StairsGateMultiBlock.hpp>
#include <Components/Ruin/StairsLowerMultiBlock.hpp>
#include <Components/Ruin/StairsSegment.hpp>
#include <Components/Ruin/StairsUpperMultiBlock.hpp>
#include <Components/SceneSettings/CurrentScene.hpp>
#include <Components/SpawnArea.hpp>
#include <Components/Spring/BarberSurgeonEntrance.hpp>
#include <Components/Spring/HealingSpringBuildingMultiBlock.hpp>
#include <Components/Spring/HealingSpringBuildingSegment.hpp>
#include <Components/Spring/HealingSpringEntrance.hpp>
#include <Components/Spring/HealingSpringMultiBlock.hpp>
#include <Components/Spring/HealingSpringSegment.hpp>
#include <Components/Wall.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/CreateNpcEvent.hpp>
#include <Events/CreatePlantEvent.hpp>
#include <Factory/CryptFactory.hpp>
#include <Factory/LootFactory.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Factory/RuinFactory.hpp>
#include <Factory/WallFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SceneControl/SceneData.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/GraveSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/ProcGen/LevelGenerator.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>

#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <ranges>
#include <spdlog/fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <tuple>
#include <unordered_map>

namespace Game::Sys::ProcGen
{

LevelGenerator::LevelGenerator( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank ),
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
  // NPCs move away from their spawn cells, so only reserve those for the duration of level generation
  for ( auto [npc_entt, npc_cmp, npc_footprint_cmp, npc_pos_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Npc::Footprint, Cmp::Position>().each() )
  {
    for ( const auto &cell_pos : npc_footprint_cmp.world_cells( npc_pos_cmp.position ) )
    {
      m_reserved_sm->remove( npc_entt, cell_pos );
    }
  }

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
  const Sprites::SpriteSheet &wall_ms = Sys::SpriteStore::instance().get( Sys::SpriteKey( scene_data.wall_tileset().name ) );
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
      Factory::Player::add_spawn_area( reg(), entity, new_pos.y - 16.0f );
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
  // Multiblock layer - only builds sprites from the tiled layer if they are explicitly included below
  using MultiblockFactoryFn = std::function<entt::entity( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet & )>;
  // an altar's kind is the scene it is built in, which decides the offering rewards it grants (see "location" in items.json)
  const auto add_altar = [this]( entt::registry &r, sf::Vector2f p, const Sprites::SpriteSheet &ss )
  {
    const auto altar_entt = Factory::Multiblock::add_multiblock_with_segments<Cmp::Altar::MultiBlock, Cmp::Altar::Segment>( r, p, ss, 0, 0, m_reserved_sm.get() ).first;
    auto kind = Cmp::Altar::Kind::GRAVEYARD;
    if ( Utils::is_current_scene( r, Cmp::SceneSettings::SceneId::CRYPT ) ) { kind = Cmp::Altar::Kind::CRYPT; }
    else if ( Utils::is_current_scene( r, Cmp::SceneSettings::SceneId::SHOP ) ) { kind = Cmp::Altar::Kind::SHOP; }
    r.patch<Cmp::Altar::MultiBlock>( altar_entt, [kind]( Cmp::Altar::MultiBlock &cmp ) { cmp.set_kind( kind ); } );
    return altar_entt;
  };
  const std::unordered_map<std::string, MultiblockFactoryFn> kMultiblockFactories{
      { "sprite.graveyard.healingspring", [this]( entt::registry &r, sf::Vector2f p, const Sprites::SpriteSheet &ss )
        { return Factory::Multiblock::add_multiblock_with_segments<Cmp::HealingSpringMultiBlock, Cmp::HealingSpringSegment>( r, p, ss, 0, 0, m_reserved_sm.get() ).first; } },
      { "sprite.graveyard.altar.inactive", add_altar },
      { "sprite.crypt.objective.closed",[this]( entt::registry &r, sf::Vector2f p, const Sprites::SpriteSheet &ss )
        { return Factory::Multiblock::add_multiblock_with_segments<Cmp::Crypt::ObjectiveMultiBlock, Cmp::Crypt::ObjectiveSegment>( r, p, ss, 0, 0, m_reserved_sm.get() ).first; } },
      { "sprite.crypt.altar.inactive", add_altar },
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
      // keep level generation from placing obstacles under the NPC - released again in cleanup_reserved()
      auto reserve_npc_cells = [this]( entt::entity new_npc_entt )
      {
        const auto &[npc_footprint_cmp, npc_pos_cmp] = reg().get<Cmp::Npc::Footprint, Cmp::Position>( new_npc_entt );
        for ( const auto &cell_pos : npc_footprint_cmp.world_cells( npc_pos_cmp.position ) )
        {
          m_reserved_sm->insert( new_npc_entt, cell_pos );
        }
      };
      get_systems_event_queue().trigger(
          Events::CreateNpcEvent( Sys::NpcKey( ms_type ), Cmp::Position( pos, Constants::kGridSizePxF ), 0, {}, reserve_npc_cells ) );
      continue;
    }

    // Item markers share this layer but are named by item id (e.g. "item.axe"), which isn't
    // itself a valid sprite key - resolve the item's actual sprite type before lookup.
    const Sys::SpriteKey sprite_lookup_type = ms_type.contains( "item." ) ? Sys::ItemStore::instance().get( Sys::ItemKey( ms_type ) ).sprite_type
                                                                          : Sys::SpriteKey( ms_type );
    const auto &ms = Sys::SpriteStore::instance().get( sprite_lookup_type );

    if ( auto it = kMultiblockFactories.find( ms_type ); it != kMultiblockFactories.end() )
    {
      auto mb_entt = it->second( reg(), pos, ms );
      m_reserved_sm->insert( mb_entt, Cmp::Position( pos, Constants::kGridSizePxF ) );
    }
    else if ( ms_type.contains( "item.plant" ) )
    {
      get_systems_event_queue().trigger( Events::CreatePlantEvent( Sys::ItemKey( ms_type ), Cmp::Position( pos, Constants::kGridSizePxF ) ) );
    }
    else if ( ms_type.contains( "item." ) )
    {
      // prevent infinite respawns in the RuinSceneUpperFloor
      auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
      if ( inventory_type == "item.witchesjar" and ms.type().contains( "witchesjar" ) ) continue;

      // make sure we mark the *world* entt as reserved
      auto world_pos_entt = Utils::get_world_pos_entt( reg(), Cmp::Position( pos, ms.sprite_size() ) );
      if ( world_pos_entt != entt::null )
      {
        m_reserved_sm->insert( world_pos_entt, Cmp::Position( pos, ms.sprite_size() ) );
        get_systems_event_queue().trigger( Events::CreateItemEvent( Cmp::Position( pos, ms.sprite_size() ), Sys::ItemKey( ms_type ) ) );
      }
    }
  }

#ifndef NDEBUG
  for ( const auto &[ms_type, pos] : scene_data.developer_objectlayer() )
  {
    // allow "npc." markers in the multiblock layer
    if ( ms_type.contains( "npc." ) )
    {
      // keep level generation from placing obstacles under the NPC - released again in cleanup_reserved()
      auto reserve_npc_cells = [this]( entt::entity new_npc_entt )
      {
        const auto &[npc_footprint_cmp, npc_pos_cmp] = reg().get<Cmp::Npc::Footprint, Cmp::Position>( new_npc_entt );
        for ( const auto &cell_pos : npc_footprint_cmp.world_cells( npc_pos_cmp.position ) )
        {
          m_reserved_sm->insert( new_npc_entt, cell_pos );
        }
      };
      get_systems_event_queue().trigger(
          Events::CreateNpcEvent( Sys::NpcKey( ms_type ), Cmp::Position( pos, Constants::kGridSizePxF ), 0, {}, reserve_npc_cells ) );
      continue;
    }

    // Item markers share this layer but are named by item id (e.g. "item.axe"), which isn't
    // itself a valid sprite key - resolve the item's actual sprite type before lookup.
    const Sys::SpriteKey sprite_lookup_type = ms_type.contains( "item." ) ? Sys::ItemStore::instance().get( Sys::ItemKey( ms_type ) ).sprite_type
                                                                          : Sys::SpriteKey( ms_type );
    const auto &ms = Sys::SpriteStore::instance().get( sprite_lookup_type );

    if ( auto it = kMultiblockFactories.find( ms_type ); it != kMultiblockFactories.end() )
    {
      auto mb_entt = it->second( reg(), pos, ms );
      m_reserved_sm->insert( mb_entt, Cmp::Position( pos, Constants::kGridSizePxF ) );
    }
    else if ( ms_type.contains( "item.plant" ) )
    {
      get_systems_event_queue().trigger( Events::CreatePlantEvent( Sys::ItemKey( ms_type ), Cmp::Position( pos, Constants::kGridSizePxF ) ) );
    }
    else if ( ms_type.contains( "item." ) )
    {
      // prevent infinite respawns in the RuinSceneUpperFloor
      auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
      if ( inventory_type == "item.witchesjar" and ms.type().contains( "witchesjar" ) ) continue;

      // make sure we mark the *world* entt as reserved
      auto world_pos_entt = Utils::get_world_pos_entt( reg(), Cmp::Position( pos, ms.sprite_size() ) );
      if ( world_pos_entt != entt::null )
      {
        m_reserved_sm->insert( world_pos_entt, Cmp::Position( pos, ms.sprite_size() ) );
        get_systems_event_queue().trigger( Events::CreateItemEvent( Cmp::Position( pos, ms.sprite_size() ), Sys::ItemKey( ms_type ) ) );
      }
    }
  }
#endif // NDEBUG
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
  const Sprites::SpriteSheet &ss_main = Sys::SpriteStore::instance().get( "sprite.graveyard.wall.int.main" );
  const Sprites::SpriteSheet &ss_cap = Sys::SpriteStore::instance().get( "sprite.graveyard.wall.int.cap" );

  decorate_obstacles( ss_main, ss_cap, [&ss_main]() { return ss_main.get_random_texture_index(); },
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
  const Sprites::SpriteSheet &ss_main = Sys::SpriteStore::instance().get( "sprite.ruin.wall.int.main" );
  const Sprites::SpriteSheet &ss_cap = Sys::SpriteStore::instance().get( "sprite.ruin.wall.int.cap" );

  decorate_obstacles( ss_main, ss_cap, []() -> std::size_t { return 0; },
                      /*cap_y_offset=*/0.f, /*moveable=*/true );
}

void LevelGenerator::add_ruin_rune_markers()
{
  constexpr int kRuneMarkerCount = 5;
  constexpr int kMaxAttempts = 1000;

  int placed = 0;
  int attempts = 0;
  while ( placed < kRuneMarkerCount and attempts < kMaxAttempts )
  {
    attempts++;
    auto [rnd_entt, rnd_pos] = Utils::Rnd::get_random_position( reg(), {}, Utils::Rnd::ExcludePack<Cmp::Player::Character>{} );
    // A random pick can land on an already-reserved position - retry with a new pick rather than
    // giving up on this rune marker, otherwise unlucky rolls silently place fewer than intended.
    if ( not m_reserved_sm->at( rnd_pos ).empty() ) continue;
    m_reserved_sm->insert( rnd_entt, rnd_pos );
    const auto &rune_ss = Sys::SpriteStore::instance().get( "sprite.ruin.runemarking.inactive" );
    auto idx = rune_ss.get_random_texture_index();
    float zorder = rune_ss.zorder( 0, rnd_pos.y() );
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
  while ( placed < num_cobwebs and attempts < kMaxAttempts )
  {
    attempts++;
    auto [rnd_entt, rnd_pos] = Utils::Rnd::get_random_position( reg(), {}, Utils::Rnd::ExcludePack<Cmp::Player::Character>{} );
    if ( rnd_entt == entt::null ) continue;
    if ( not m_reserved_sm->at( rnd_pos ).empty() ) continue;

    if ( has_collision( Cmp::RectBounds::scaled( { rnd_pos.position }, gridsize, 1 ) ) ) continue;
    const auto &cobweb_ss = Sys::SpriteStore::instance().get( "sprite.ruin.cobweb" );
    Factory::Ruin::create_cobweb( reg(), rnd_entt, rnd_pos.position, cobweb_ss, cobweb_ss.get_random_texture_index(), *m_reserved_sm );
    m_non_obstacle_sm->insert( rnd_entt, rnd_pos );
    placed++;
  }
}

void LevelGenerator::gen_graveyard_exterior_multiblocks()
{
  // const size_t grave_num_multiplier = Sys::PersistSystem::get<Cmp::Persist::GraveNumMultiplier>( reg() ).get_value();
  // only one altar can be activated at a time, so more altars mean less backtracking, not more graves (see below)
  const auto max_num_altars = static_cast<size_t>(
      std::max( 1, static_cast<int>( Sys::PersistSystem::get<Cmp::Persist::NumAltars>( reg() ).get_value() ) ) );
  const size_t max_num_crypts = 1;
  // one healing building per level: either a healing spring or a barber surgeon
  const size_t max_num_healing_buildings = 1;
  const auto &min_num_ruins = Sys::PersistSystem::get<Cmp::Persist::MinNumRuins>( reg() );

  // minimum clearance (in grid cells, both axes) between altars, crypts, healing springs and ruins
  constexpr int kMinBuildingSpacingCells = 10;

  // spawn-location lookup for the graves; a failure is reported once, by the grave count check
  auto find_spawn_pos = [&]( const Sprites::SpriteSheet &ms ) -> std::optional<Cmp::Position>
  {
    auto [random_entity, random_origin_position] = find_spawn_location( ms, 0 );
    if ( random_entity == entt::null ) { return std::nullopt; }
    return random_origin_position;
  };

  // ALTARS
  const auto &altar1_spritesheet = Sys::SpriteStore::instance().get( "sprite.graveyard.altar.inactive" );
  auto altar_mb_entts = spawn_multiblocks<Cmp::Altar::MultiBlock, Cmp::Altar::Segment>( max_num_altars, altar1_spritesheet,
                                                                                        /*log=*/true, kMinBuildingSpacingCells );
  for ( auto entt : altar_mb_entts )
  {
    // every altar takes one artefact of each group
    reg().emplace_or_replace<Cmp::Altar::ArtefactCount>( entt );
  }

  // CRYPTS - one crypt serves every grave trail's crypt key, whichever altar started it
  const auto &crypt_spritesheet = Sys::SpriteStore::instance().get( "sprite.graveyard.crypt.closed" );
  spawn_multiblocks<Cmp::Crypt::BuildingMultiBlock, Cmp::Crypt::BuildingSegment>( max_num_crypts, crypt_spritesheet,
                                                                                  /*log=*/true, kMinBuildingSpacingCells );

  // HEALING BUILDING - the healing spring and the barber surgeon share the exterior building, only the entrance differs
  const auto &healingspring_spritesheet = Sys::SpriteStore::instance().get( "sprite.graveyard.building.healingspring" );
  auto healing_mb_entts = spawn_multiblocks<Cmp::HealingSpringBuildingMultiBlock, Cmp::HealingSpringBuildingSegment>(
      max_num_healing_buildings, healingspring_spritesheet, /*log=*/true, kMinBuildingSpacingCells );
  for ( auto mb_entt : healing_mb_entts )
  {
    // 0 = healing spring, 1 = barber surgeon; anything else is a 50/50 roll
    int building_kind = Sys::PersistSystem::get<Cmp::Persist::HealingBuildingOverride>( reg() ).get_value();
    if ( building_kind != 0 and building_kind != 1 ) { building_kind = Cmp::RandomInt( 0, 1 ).gen(); }
    const bool is_barber = building_kind == 1;
    SPDLOG_INFO( "Healing building is a {}", is_barber ? "Barber Surgeon" : "Healing Spring" );
    if ( not is_barber ) continue;

    const auto *mb_uuid = reg().try_get<Cmp::UUID>( mb_entt );
    if ( mb_uuid == nullptr ) continue;
    for ( auto [door_entt, door_cmp, door_uuid] : reg().view<Cmp::HealingSpringEntrance, Cmp::UUID>().each() )
    {
      if ( door_uuid != *mb_uuid ) continue;
      reg().emplace_or_replace<Cmp::BarberSurgeonEntrance>( door_entt );
    }
  }

  const auto &ruin_spritesheet = Sys::SpriteStore::instance().get( "sprite.graveyard.ruin" );
  spawn_multiblocks<Cmp::Ruin::BuildingMultiBlock, Cmp::Ruin::BuildingSegment>( static_cast<std::size_t>( min_num_ruins.get_value() ),
                                                                                ruin_spritesheet,
                                                                                /*log=*/true, kMinBuildingSpacingCells );

  // GRAVES - placed last, so the larger buildings above get first pick of the space
  auto grave_meta_types = Sys::SpriteStore::instance().get_pattern( R"(graves\.\w+\.closed$)" );
  if ( grave_meta_types.size() < 2 ) { SPDLOG_WARN( "No GRAVE spritesheets found in SpriteStore" ); }
  else
  {
    const size_t trail_size = Sys::PersistSystem::get<Cmp::Persist::GraveTrailNominalHopCount>( reg() ).get_value();
    const size_t max_cadaver_count = Utils::Player::get_cadaver( reg() ).size();
    // const size_t level_depth = Utils::Player::get_level_depth( reg() ).get_count();
    const size_t exit_and_crypt_key_multiplier = 2;
    const size_t trail_start_count = 1;
    // sized per cadaver part, not per altar: each part takes one altar activation and one trail, whichever altar is used.
    // Sized for the longest trail of the run, at the deepest level
    const auto max_level_depth = static_cast<size_t>( std::max( 1, Sys::PersistSystem::get<Cmp::Persist::MaxLevelDepth>( reg() ).get_value() ) );
    const size_t altar_activation_count = Cmp::Altar::ArtefactCount::size();
    // trail graves skip their consequence, so each altar activation's artefacts are dug from graves off the trail.
    // Sized for the worst case: the artefact chance never drops below Cmp::Persist::ArtefactMinChance
    // the odds line is fixed per level, so settings changes take effect from the next level
    using Slot = Cmp::Grave::Consequence::Slot;
    using ConsequenceType = Cmp::Grave::Consequence::Type;
    const auto consequence_line = Cmp::Grave::Consequence::make_line( {
        Slot{ .type = ConsequenceType::CURSE_TABLET, .value = Sys::PersistSystem::get<Cmp::Persist::GraveOddsCurseTablet>( reg() ).get_value() },
        Slot{ .type = ConsequenceType::BOMB_TRAP, .value = Sys::PersistSystem::get<Cmp::Persist::GraveOddsBombTrap>( reg() ).get_value() },
        Slot{ .type = ConsequenceType::TORCH, .value = Sys::PersistSystem::get<Cmp::Persist::GraveOddsTorch>( reg() ).get_value() },
        Slot{ .type = ConsequenceType::NPC_TRAP, .value = Sys::PersistSystem::get<Cmp::Persist::GraveOddsNpcTrap>( reg() ).get_value() },
        Slot{ .type = ConsequenceType::ARTEFACT, .value = Sys::PersistSystem::get<Cmp::Persist::GraveOddsArtefact>( reg() ).get_value() },
        Slot{ .type = ConsequenceType::TALISMAN, .value = Sys::PersistSystem::get<Cmp::Persist::GraveOddsTalisman>( reg() ).get_value() },
    } );
    // Consequence caps the floor at the artefact band width, so size for the capped value
    int max_artefact_floor_pct = Cmp::Grave::Consequence::max_artefact_floor_pct( consequence_line );
    if ( max_artefact_floor_pct < 1 )
    {
      SPDLOG_WARN( "Grave odds leave no band for artefacts (width {}): the artefact min chance cannot be guaranteed", max_artefact_floor_pct );
      max_artefact_floor_pct = 1;
    }
    const int min_artefact_chance_pct = std::clamp( Sys::PersistSystem::get<Cmp::Persist::ArtefactMinChance>( reg() ).get_value(), 1,
                                                    max_artefact_floor_pct );
    const auto min_artefact_chance = static_cast<size_t>( min_artefact_chance_pct );
    const size_t artefact_graves = ( altar_activation_count * 100 + min_artefact_chance - 1 ) / min_artefact_chance;
    const size_t trail_graves = ( trail_size + max_level_depth ) * exit_and_crypt_key_multiplier + trail_start_count;
    const size_t base_num_graves = ( trail_graves + artefact_graves ) * max_cadaver_count;
    // the artefact graves above are only the average needed, and bombs blast graves open, so add a margin on top
    const auto margin_pct = static_cast<size_t>( std::max( 0, Sys::PersistSystem::get<Cmp::Persist::GraveCountMargin>( reg() ).get_value() ) );
    const size_t required_num_graves = ( base_num_graves * ( 100 + margin_pct ) + 99 ) / 100;
    SPDLOG_INFO( "Graves required: {} = ({} trail + {} artefact per cadaver part) x {} parts + {}% margin", required_num_graves, trail_graves,
                 artefact_graves, max_cadaver_count, margin_pct );

    std::map<Cmp::Grave::Consequence::Type, int> grave_consequences_distribution;
    std::size_t placed_num_graves = 0;
    for ( std::size_t i = 0; i < required_num_graves; ++i )
    {
      const auto &spritesheet = Sys::SpriteStore::instance().get_random( grave_meta_types );
      auto index = spritesheet.get_random_texture_index();
      SPDLOG_DEBUG( "Selected {}, {}", spritesheet.get_sprite_type(), index );
      auto pos = find_spawn_pos( spritesheet );
      // find_spawn_location() already made many random attempts, so the graveyard is full: further graves will not fit either
      if ( not pos ) break;

      auto [mb_entt, _] = Factory::Multiblock::add_multiblock_with_segments<Cmp::Grave::MultiBlock, Cmp::Grave::Segment>(
          reg(), pos->position, spritesheet, index, 0, m_reserved_sm.get() );
      m_reserved_sm->insert( mb_entt, pos.value() );
      // fix the roll at creation; the consequence is resolved against the player's current luck when needed
      auto &consequence = reg().emplace_or_replace<Cmp::Grave::Consequence>( mb_entt, Cmp::RandomInt( 0, 100 ).gen(), min_artefact_chance_pct,
                                                                             consequence_line );
      // simulate the rolls now to get an idea of the distribution with game start player luck
      grave_consequences_distribution[consequence.roll( Utils::Player::get_stats( reg() ).luck() )]++;
      placed_num_graves++;
    }
    if ( placed_num_graves < base_num_graves )
    {
      SPDLOG_ERROR( "Only added {} of the {} graves required: the level may run out of graves before every cadaver part is found", placed_num_graves,
                    base_num_graves );
    }
    else if ( placed_num_graves < required_num_graves )
    {
      SPDLOG_ERROR( "Only added {} of the {} graves required with the {}% margin (baseline of {} met)", placed_num_graves, required_num_graves,
                    margin_pct, base_num_graves );
    }
    else { SPDLOG_INFO( "Added {} graves", placed_num_graves ); }
    SPDLOG_INFO( "Grave Consequences dist at current luck: {}", grave_consequences_distribution );
  }
}

template <typename MULTIBLOCK, typename MBSEGMENT>
std::vector<entt::entity> LevelGenerator::spawn_multiblocks( std::size_t count, const Sprites::SpriteSheet &ss, bool log, int min_spacing_cells )
{
  std::vector<entt::entity> entts_created;
  for ( std::size_t i = 0; i < count; ++i )
  {
    // Try the requested spacing first, then halve it until a position is found, so the
    // requested count is still met when the map is too crowded for the full spacing.
    int spacing = min_spacing_cells;
    auto [random_entity, random_origin_position] = find_spawn_location( ss, 0, spacing );
    while ( random_entity == entt::null and spacing > 0 )
    {
      spacing /= 2;
      SPDLOG_WARN( "No spawn position for {} with requested spacing, retrying with {} grid cells.", ss.type(), spacing );
      std::tie( random_entity, random_origin_position ) = find_spawn_location( ss, 0, spacing );
    }
    if ( random_entity == entt::null )
    {
      SPDLOG_ERROR( "Failed to find valid spawn position for {}.", ss.type() );
      continue;
    }

    auto [mb_entt, _] = Factory::Multiblock::add_multiblock_with_segments<MULTIBLOCK, MBSEGMENT>( reg(), random_origin_position.position, ss, 0, 0,
                                                                                                  m_reserved_sm.get() );
    m_reserved_sm->insert( mb_entt, random_origin_position );
    if ( log ) { SPDLOG_INFO( "Added {} to {},{}", ss.type(), random_origin_position.position.x, random_origin_position.position.y ); }
    entts_created.push_back( mb_entt );
  }
  return entts_created;
}

std::pair<entt::entity, Cmp::Position> LevelGenerator::find_spawn_location( const Sprites::SpriteSheet &ms, unsigned long seed,
                                                                            int min_spacing_cells )
{
  constexpr int kMaxAttempts = 1000;
  int attempts = 0;
  unsigned long current_seed = seed;

  while ( attempts < kMaxAttempts )
  {
    auto [random_entity, random_pos] = Utils::Rnd::get_random_position( reg(), Utils::Rnd::IncludePack<>{},
                                                                        Utils::Rnd::ExcludePack<Cmp::Wall, Cmp::Player::Character>{}, current_seed );

    auto lo_sprite_size = Sys::SpriteStore::instance().get( ms.type() ).sprite_size();
    auto new_lo_hitbox = Cmp::RectBounds::scaled( random_pos.position, lo_sprite_size, 1.f );

    // Check collisions with walls, graves, shrines, the exit, and anything else already claiming this position
    auto is_valid = [&]() -> bool
    {
      using Utils::Collision::any_intersects;
      return not any_intersects<Cmp::Wall>( reg(), new_lo_hitbox ) and not any_intersects<Cmp::Grave::Segment>( reg(), new_lo_hitbox ) and
             not any_intersects<Cmp::Altar::Segment>( reg(), new_lo_hitbox ) and
             not any_intersects<Cmp::Crypt::BuildingSegment>( reg(), new_lo_hitbox ) and
             not any_intersects<Cmp::HealingSpringBuildingSegment>( reg(), new_lo_hitbox ) and
             not any_intersects<Cmp::Ruin::BuildingSegment>( reg(), new_lo_hitbox ) and
             not any_intersects<Cmp::Crypt::ObjectiveSegment>( reg(), new_lo_hitbox ) and
             not any_intersects<Cmp::Grave::ExitSegment>( reg(), new_lo_hitbox ) and
             m_reserved_sm->query_rect( new_lo_hitbox.getBounds() ).empty() and not any_intersects<Cmp::SpawnArea>( reg(), new_lo_hitbox ) and
             not any_intersects<Cmp::Player::Character>( reg(), new_lo_hitbox );
    };

    // Keep the large graveyard buildings at least `min_spacing_cells` apart in both axes, and away from the exit
    auto is_spaced = [&]() -> bool
    {
      if ( min_spacing_cells <= 0 ) return true;
      using Utils::Collision::any_intersects;
      auto spacing_hitbox = Cmp::RectBounds::expanded( random_pos.position, lo_sprite_size, min_spacing_cells );
      return not(
          any_intersects<Cmp::Altar::Segment>( reg(), spacing_hitbox ) || any_intersects<Cmp::Crypt::BuildingSegment>( reg(), spacing_hitbox ) ||
          any_intersects<Cmp::HealingSpringBuildingSegment>( reg(), spacing_hitbox ) ||
          any_intersects<Cmp::Ruin::BuildingSegment>( reg(), spacing_hitbox ) || any_intersects<Cmp::Grave::ExitSegment>( reg(), spacing_hitbox ) );
    };

    if ( is_valid() and is_spaced() )
    {
      if ( current_seed != seed and seed > 0 )
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

std::vector<entt::entity> LevelGenerator::gen_loot_containers( sf::Vector2u map_grid_size )
{
  std::vector<entt::entity> assigned_entts;

  auto num_loot_containers = map_grid_size.x * map_grid_size.y / 120; // one loot container per N grid squares

  // keep sampling until the target is met - candidates that land on reserved cells are rejected
  const std::size_t max_attempts = num_loot_containers * 20;
  for ( std::size_t attempts = 0; assigned_entts.size() < num_loot_containers and attempts < max_attempts; ++attempts )
  {
    auto [random_entity,
          random_origin_position] = Utils::Rnd::get_random_position( reg(), {}, Utils::Rnd::ExcludePack<Cmp::Player::Character, Cmp::Obstacle>{}, 0 );

    if ( m_reserved_sm->at( random_origin_position ).empty() )
    {
      float zorder = Sys::SpriteStore::instance().get( "sprite.graveyard.pots" ).sprite_size().y;

      Cmp::RandomInt pot_picker( 0, 2 );
      Factory::Loot::create_loot_container( reg(), random_entity, random_origin_position, "sprite.graveyard.pots", pot_picker.gen(), zorder );
      assigned_entts.push_back( random_entity );
      m_reserved_sm->insert( random_entity, random_origin_position );
    }
  }
  if ( assigned_entts.size() < num_loot_containers )
  {
    SPDLOG_WARN( "Only placed {} of {} loot containers after {} attempts.", assigned_entts.size(), num_loot_containers, max_attempts );
  }
  SPDLOG_INFO( "Added {} loot containers.", assigned_entts.size() );
  return assigned_entts;
}

std::vector<entt::entity> LevelGenerator::gen_npc_containers( sf::Vector2u map_grid_size )
{
  std::vector<entt::entity> assigned_entts;

  // full density (one NPC container per N grid squares) is only reached at the final level depth;
  // shallower levels get a logarithmic fraction of it: log(1 + depth) / log(1 + max_depth),
  // so counts start high and flatten out towards the final depth.
  // For the 99x99 graveyard map (81 at full density) with MaxLevelDepth = 9:
  //   depth: 1   2   3   4   5   6   7   8   9
  //   npcs: 24  39  49  57  63  68  73  77  81
  const std::size_t max_npc_containers = map_grid_size.x * map_grid_size.y / 120;
  const auto max_level_depth = static_cast<std::size_t>( std::max( 1, Sys::PersistSystem::get<Cmp::Persist::MaxLevelDepth>( reg() ).get_value() ) );
  const auto level_depth = std::clamp<std::size_t>( Utils::Player::get_level_depth( reg() ).get_count(), 1, max_level_depth );
  const double depth_ratio = std::log1p( static_cast<double>( level_depth ) ) / std::log1p( static_cast<double>( max_level_depth ) );
  const std::size_t num_npc_containers = std::max<std::size_t>(
      1, static_cast<std::size_t>( std::lround( static_cast<double>( max_npc_containers ) * depth_ratio ) ) );

  // keep sampling until the target is met - candidates that land on reserved cells are rejected
  const std::size_t max_attempts = num_npc_containers * 20;
  for ( std::size_t attempts = 0; assigned_entts.size() < num_npc_containers and attempts < max_attempts; ++attempts )
  {
    auto [random_entity,
          random_origin_position] = Utils::Rnd::get_random_position( reg(), {}, Utils::Rnd::ExcludePack<Cmp::Player::Character, Cmp::Obstacle>{}, 0 );

    if ( m_reserved_sm->at( random_origin_position ).empty() )
    {
      // pick a random texture index for the npc container
      const Sys::SpriteKey npc_type = "sprite.graveyard.bones";
      auto rand_npc_tex_idx = Sys::SpriteStore::instance().get( npc_type ).get_random_texture_index();

      Factory::Npc::create_npc_container( reg(), random_entity, random_origin_position, npc_type, rand_npc_tex_idx, 0.f );
      assigned_entts.push_back( random_entity );
      m_reserved_sm->insert( random_entity, random_origin_position );
    }
  }
  if ( assigned_entts.size() < num_npc_containers )
  {
    SPDLOG_WARN( "Only placed {} of {} NPC containers after {} attempts.", assigned_entts.size(), num_npc_containers, max_attempts );
  }
  SPDLOG_INFO( "Added {} NPC containers (level depth {} of {}).", assigned_entts.size(), level_depth, max_level_depth );
  return assigned_entts;
}

std::vector<entt::entity> LevelGenerator::gen_random_plants()
{
  std::vector<entt::entity> assigned_entts;

  constexpr int kMinCountPerPlantType = 10;

  // Find all plant sprites. The map value will track count of each plant type added.
  std::unordered_map<Sys::SpriteKey, int> plant_tracker;
  plant_tracker = Sys::SpriteStore::instance().get_pattern( R"(sprite\.item\.plant\..*)" ) |
                  std::views::transform( []( const auto &type ) { return std::pair{ type, 0 }; } ) |
                  std::ranges::to<std::unordered_map<Sys::SpriteKey, int>>();

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

    // PlantSystem re-adds the "sprite." prefix itself (to match the bare "item.plant.*" markers used elsewhere), so strip it back off here.
    const std::string kSpritePrefix = "sprite.";
    const Sys::ItemKey chosen_plant_item_type( selected_plant_iter->first.str().substr( kSpritePrefix.size() ) );

    // Create the plant in the game area and increment the tracker counter
    bool created = false;
    get_systems_event_queue().trigger(
        Events::CreatePlantEvent( chosen_plant_item_type, random_pos, [&created]( [[maybe_unused]] entt::entity plant_entt ) { created = true; } ) );
    if ( created )
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