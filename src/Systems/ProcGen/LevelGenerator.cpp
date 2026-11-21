#include <Components/Altar/BloodCount.hpp>
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
#include <Components/Persistent/BloodMinChance.hpp>
#include <Components/Persistent/GraveClusterExternalPadding.hpp>
#include <Components/Persistent/GraveClusterGridSize.hpp>
#include <Components/Persistent/GraveNumMultiplier.hpp>
#include <Components/Persistent/GraveOddsBlood.hpp>
#include <Components/Persistent/GraveOddsBombTrap.hpp>
#include <Components/Persistent/GraveOddsCurseTablet.hpp>
#include <Components/Persistent/GraveOddsNpcTrap.hpp>
#include <Components/Persistent/GraveOddsTalisman.hpp>
#include <Components/Persistent/GraveOddsTorch.hpp>
#include <Components/Persistent/HealingBuildingOverride.hpp>
#include <Components/Persistent/MaxLevelDepth.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
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
#include <Components/UUID.hpp>
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
#include <cstdint>
#include <functional>
#include <ranges>
#include <string_view>
#include <spdlog/fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace Game::Sys::ProcGen
{

namespace
{

//! @brief Unique key for the grid cell containing a pixel position.
std::int64_t cell_key( sf::Vector2f p )
{
  return ( static_cast<std::int64_t>( std::floor( p.x / Constants::kGridSizePxF.x ) ) << 32 ) bitor
         static_cast<std::uint32_t>( static_cast<std::int32_t>( std::floor( p.y / Constants::kGridSizePxF.y ) ) );
}

} // namespace

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

PathFinding::SpatialHashGrid &LevelGenerator::get_claims_sm() { return m_claims_sm; }

bool LevelGenerator::is_cell_taken( const Cmp::Position &pos ) const
{
  if ( not m_reserved_sm->at( pos ).empty() or not m_claims_sm.at( pos ).empty() ) return true;
  const auto item_sm = m_item_sm.lock();
  return item_sm and not item_sm->at( pos ).empty();
}

bool LevelGenerator::is_area_taken( const sf::FloatRect &bounds ) const
{
  if ( not m_reserved_sm->query_rect( bounds ).empty() or not m_claims_sm.query_rect( bounds ).empty() ) return true;
  const auto item_sm = m_item_sm.lock();
  return item_sm and not item_sm->query_rect( bounds ).empty();
}

void LevelGenerator::discard_claims() { m_claims_sm.clear(); }

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

  // Paths
  if ( not scene_data.path_tileset().name.empty() )
  {
    const Sprites::SpriteSheet &path_ms = Sys::SpriteStore::instance().get( Sys::SpriteKey( scene_data.path_tileset().name ) );
    for ( const auto [i, tile] : std::views::enumerate( scene_data.path_tilelayer() ) )
    {
      auto row = i / w; // increments every 'w' tiles
      auto col = i % w; // wraps back to zero every 'w' tiles
      sf::Vector2f new_pos( static_cast<float>( col ) * Constants::kGridSizePxF.x, static_cast<float>( row ) * Constants::kGridSizePxF.y );
      if ( tile >= scene_data.path_tileset().first_gid )
      {
        // get the relative index using the offset from the json data
        auto entt = Factory::Wall::add_path_entity( reg(), new_pos, path_ms, tile - scene_data.path_tileset().first_gid );
        // reserve it so obstacle/cellular-automata procgen doesn't repurpose the path entity (and overwrite its sprite)
        m_reserved_sm->insert( entt, Cmp::Position( new_pos, Constants::kGridSizePxF ) );
      }
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
  // The add_solid_* factories only tag entities that already exist under the rect, and an empty cell (e.g. a
  // levelgen "wall" tile with no wall-layer tile) is walkable. So give every empty cell under a solid its own
  // entity first - the factories then make it block like any other solid cell.
  std::unordered_set<std::int64_t> occupied_cells;
  for ( auto [entt, pos_cmp] : reg().view<Cmp::Position>().each() )
  {
    occupied_cells.insert( cell_key( pos_cmp.getCenter() ) );
  }
  for ( const auto &solid : scene_data.solid_objectlayer() )
  {
    const auto first_col = static_cast<int>( std::floor( solid.position.x / Constants::kGridSizePxF.x ) );
    const auto first_row = static_cast<int>( std::floor( solid.position.y / Constants::kGridSizePxF.y ) );
    const auto last_col = static_cast<int>( std::ceil( ( solid.position.x + solid.size.x ) / Constants::kGridSizePxF.x ) );
    const auto last_row = static_cast<int>( std::ceil( ( solid.position.y + solid.size.y ) / Constants::kGridSizePxF.y ) );
    for ( int row = first_row; row < last_row; ++row )
    {
      for ( int col = first_col; col < last_col; ++col )
      {
        const sf::Vector2f cell_pos( static_cast<float>( col ) * Constants::kGridSizePxF.x, static_cast<float>( row ) * Constants::kGridSizePxF.y );
        const sf::Vector2f cell_center = cell_pos + ( Constants::kGridSizePxF / 2.f );
        // same center-containment rule as the add_solid_* factories
        if ( not solid.contains( cell_center ) ) continue;
        // void cells are outside the playable area already, so don't fill them with entities
        const Cmp::Position cell_pos_cmp( cell_pos, Constants::kGridSizePxF );
        if ( not m_void_sm->at( cell_pos_cmp ).empty() ) continue;
        if ( not occupied_cells.insert( cell_key( cell_center ) ).second ) continue;
        reg().emplace<Cmp::Position>( reg().create(), cell_pos_cmp );
      }
    }
  }
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
    // the graveyard altar fills its blood quota before it activates
    if ( kind == Cmp::Altar::Kind::GRAVEYARD ) { r.emplace_or_replace<Cmp::Altar::BloodCount>( altar_entt ); }
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
      // keep level generation from placing obstacles under the NPC - claims only last for this generation pass
      auto reserve_npc_cells = [this]( entt::entity new_npc_entt )
      {
        const auto &[npc_footprint_cmp, npc_pos_cmp] = reg().get<Cmp::Npc::Footprint, Cmp::Position>( new_npc_entt );
        for ( const auto &cell_pos : npc_footprint_cmp.world_cells( npc_pos_cmp.position ) )
        {
          m_claims_sm.insert( new_npc_entt, cell_pos );
        }
      };
      get_systems_event_queue().trigger(
          Events::CreateNpcEvent( Sys::NpcKey( ms_type ), Cmp::Position( pos, Constants::kGridSizePxF ), 0, {}, reserve_npc_cells ) );
      continue;
    }

    // Item markers share this layer but are named by item id (e.g. "item.shovel"), which isn't
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

      // only place items on real world positions; ItemSystem indexes the item in the scene's item grid, which
      // is_cell_taken() checks, so later generation passes won't cover it
      auto world_pos_entt = Utils::get_world_pos_entt( reg(), Cmp::Position( pos, ms.sprite_size() ) );
      if ( world_pos_entt != entt::null )
      {
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
      // keep level generation from placing obstacles under the NPC - claims only last for this generation pass
      auto reserve_npc_cells = [this]( entt::entity new_npc_entt )
      {
        const auto &[npc_footprint_cmp, npc_pos_cmp] = reg().get<Cmp::Npc::Footprint, Cmp::Position>( new_npc_entt );
        for ( const auto &cell_pos : npc_footprint_cmp.world_cells( npc_pos_cmp.position ) )
        {
          m_claims_sm.insert( new_npc_entt, cell_pos );
        }
      };
      get_systems_event_queue().trigger(
          Events::CreateNpcEvent( Sys::NpcKey( ms_type ), Cmp::Position( pos, Constants::kGridSizePxF ), 0, {}, reserve_npc_cells ) );
      continue;
    }

    // Item markers share this layer but are named by item id (e.g. "item.shovel"), which isn't
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

      // only place items on real world positions; ItemSystem indexes the item in the scene's item grid, which
      // is_cell_taken() checks, so later generation passes won't cover it
      auto world_pos_entt = Utils::get_world_pos_entt( reg(), Cmp::Position( pos, ms.sprite_size() ) );
      if ( world_pos_entt != entt::null )
      {
        get_systems_event_queue().trigger( Events::CreateItemEvent( Cmp::Position( pos, ms.sprite_size() ), Sys::ItemKey( ms_type ) ) );
      }
    }
  }
#endif // NDEBUG
}

void LevelGenerator::try_place_obstacle( entt::entity entity, const Cmp::Position &pos_cmp, float init_chance )
{
  if ( Cmp::RandomFloat{ 0.f, 1.f }.gen() >= init_chance ) return;
  if ( not Factory::Obstacle::add_obstacle( reg(), entity ) ) return;
  m_obstacle_sm->insert( entity, pos_cmp );
  m_claims_sm.insert( entity, pos_cmp );
}

void LevelGenerator::add_graveyard_exterior_obstacles( float init_chance )
{
  auto position_view = reg().view<Cmp::Position>( entt::exclude<Cmp::Player::Character> );
  for ( auto [entity, pos_cmp] : position_view.each() )
  {
    if ( is_cell_taken( pos_cmp ) ) continue;
    try_place_obstacle( entity, pos_cmp, init_chance );
  }
}

void LevelGenerator::decorate_obstacles( const Sprites::SpriteSheet &ss_main, const Sprites::SpriteSheet &ss_cap,
                                         const std::function<std::size_t()> &pick_index, float cap_y_offset, bool moveable )
{
  // Note: entities with Cmp::Obstacle here are only claimed (try_place_obstacle inserts them into
  // m_claims_sm to block *other* candidates from being picked), never skipped, so every one of them
  // is decorated here.
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
    m_claims_sm.insert( cap_entt, cap_position );
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
    if ( not is_cell_taken( pos_cmp ) ) { try_place_obstacle( entity, pos_cmp, init_chance ); }
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
    if ( is_cell_taken( rnd_pos ) ) continue;
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
    if ( is_cell_taken( rnd_pos ) ) continue;

    if ( has_collision( Cmp::RectBounds::scaled( { rnd_pos.position }, gridsize, 1 ) ) ) continue;
    const auto &cobweb_ss = Sys::SpriteStore::instance().get( "sprite.ruin.cobweb" );
    Factory::Ruin::create_cobweb( reg(), rnd_entt, rnd_pos.position, cobweb_ss, cobweb_ss.get_random_texture_index(), *m_reserved_sm );
    m_non_obstacle_sm->insert( rnd_entt, rnd_pos );
    placed++;
  }
}

void LevelGenerator::gen_graveyard_exterior_multiblocks( const Scene::SceneData &scene_data, sf::Vector2f crypt_pos, sf::Vector2f healing_pos,
                                                         sf::Vector2f ruin_pos )
{
  // the altar is not spawned here: graveyard.json places it, see build_scene_from_data()

  // CRYPT - one crypt serves every key grave's crypt key, whichever altar started it
  const auto &crypt_spritesheet = Sys::SpriteStore::instance().get( "sprite.graveyard.crypt.closed" );
  spawn_multiblock_at<Cmp::Crypt::BuildingMultiBlock, Cmp::Crypt::BuildingSegment>( crypt_pos, crypt_spritesheet );

  // HEALING BUILDING - the healing spring and the barber surgeon share the exterior building, only the entrance differs
  const auto &healingspring_spritesheet = Sys::SpriteStore::instance().get( "sprite.graveyard.building.healingspring" );
  const auto healing_mb_entt = spawn_multiblock_at<Cmp::HealingSpringBuildingMultiBlock, Cmp::HealingSpringBuildingSegment>(
      healing_pos, healingspring_spritesheet );
  {
    // 0 = healing spring, 1 = barber surgeon; anything else is a 50/50 roll
    int building_kind = Sys::PersistSystem::get<Cmp::Persist::HealingBuildingOverride>( reg() ).get_value();
    if ( building_kind != 0 and building_kind != 1 ) { building_kind = Cmp::RandomInt( 0, 1 ).gen(); }
    const bool is_barber = building_kind == 1;
    SPDLOG_INFO( "Healing building is a {}", is_barber ? "Barber Surgeon" : "Healing Spring" );
    const auto *mb_uuid = reg().try_get<Cmp::UUID>( healing_mb_entt );
    if ( is_barber and mb_uuid != nullptr )
    {
      for ( auto [door_entt, door_cmp, door_uuid] : reg().view<Cmp::HealingSpringEntrance, Cmp::UUID>().each() )
      {
        if ( door_uuid != *mb_uuid ) continue;
        reg().emplace_or_replace<Cmp::BarberSurgeonEntrance>( door_entt );
      }
    }
  }

  // RUIN
  const auto &ruin_spritesheet = Sys::SpriteStore::instance().get( "sprite.graveyard.ruin" );
  spawn_multiblock_at<Cmp::Ruin::BuildingMultiBlock, Cmp::Ruin::BuildingSegment>( ruin_pos, ruin_spritesheet );

  // GRAVES - placed last, so the larger buildings above get first pick of the space
  for ( auto &padding : m_grave_cluster_padding )
  {
    padding.clear();
  }
  auto grave_meta_types = Sys::SpriteStore::instance().get_pattern( R"(graves\.\w+\.closed$)" );
  if ( grave_meta_types.size() < 2 ) { SPDLOG_WARN( "No GRAVE spritesheets found in SpriteStore" ); }
  else
  {
    // the odds line is fixed per level, so settings changes take effect from the next level
    using Slot = Cmp::Grave::Consequence::Slot;
    using ConsequenceType = Cmp::Grave::Consequence::Type;
    const auto consequence_line = Cmp::Grave::Consequence::make_line( {
        Slot{ .type = ConsequenceType::CURSE_TABLET, .value = Sys::PersistSystem::get<Cmp::Persist::GraveOddsCurseTablet>( reg() ).get_value() },
        Slot{ .type = ConsequenceType::BOMB_TRAP, .value = Sys::PersistSystem::get<Cmp::Persist::GraveOddsBombTrap>( reg() ).get_value() },
        Slot{ .type = ConsequenceType::TORCH, .value = Sys::PersistSystem::get<Cmp::Persist::GraveOddsTorch>( reg() ).get_value() },
        Slot{ .type = ConsequenceType::NPC_TRAP, .value = Sys::PersistSystem::get<Cmp::Persist::GraveOddsNpcTrap>( reg() ).get_value() },
        Slot{ .type = ConsequenceType::BLOOD, .value = Sys::PersistSystem::get<Cmp::Persist::GraveOddsBlood>( reg() ).get_value() },
        Slot{ .type = ConsequenceType::TALISMAN, .value = Sys::PersistSystem::get<Cmp::Persist::GraveOddsTalisman>( reg() ).get_value() },
    } );
    // Consequence caps the floor at the blood band width
    int max_blood_floor_pct = Cmp::Grave::Consequence::max_blood_floor_pct( consequence_line );
    if ( max_blood_floor_pct < 1 )
    {
      SPDLOG_WARN( "Grave odds leave no band for blood (width {}): the blood min chance cannot be guaranteed", max_blood_floor_pct );
      max_blood_floor_pct = 1;
    }
    const int min_blood_chance_pct = std::clamp( Sys::PersistSystem::get<Cmp::Persist::BloodMinChance>( reg() ).get_value(), 1, max_blood_floor_pct );

    // cluster geometry, in grid cells
    const int slots_per_side = std::max( 1, Sys::PersistSystem::get<Cmp::Persist::GraveClusterGridSize>( reg() ).get_value() );
    const int external_padding = std::max( 0, Sys::PersistSystem::get<Cmp::Persist::GraveClusterExternalPadding>( reg() ).get_value() );
    const int slot_stride = kGraveSlotCells + kGraveClusterInternalPadding;
    const int cluster_cells = ( slots_per_side * kGraveSlotCells ) + ( ( slots_per_side - 1 ) * kGraveClusterInternalPadding );
    const int cluster_stride = cluster_cells + external_padding;

    // quadrant cell ranges, split where SceneData::quadrant_of() splits: cell c is in the right/bottom half when c >= size / 2
    const auto [map_size_grid, _] = scene_data.map_size();
    const std::array<int, 2> split{ ( static_cast<int>( map_size_grid.x ) + 1 ) / 2, ( static_cast<int>( map_size_grid.y ) + 1 ) / 2 };
    const std::array<int, 2> map_cells{ static_cast<int>( map_size_grid.x ), static_cast<int>( map_size_grid.y ) };

    const auto cell_px = []( int col, int row )
    { return sf::Vector2f( static_cast<float>( col ) * Constants::kGridSizePxF.x, static_cast<float>( row ) * Constants::kGridSizePxF.y ); };

    std::map<Cmp::Grave::Consequence::Type, int> grave_consequences_distribution;
    std::size_t placed_num_graves = 0;
    for ( std::size_t quadrant = 0; quadrant < m_grave_cluster_padding.size(); ++quadrant )
    {
      // quadrant index is right + 2 * bottom, see SceneData::quadrant_of()
      const bool right = ( quadrant % 2 ) == 1;
      const bool bottom = ( quadrant / 2 ) == 1;
      const int q_first_col = right ? split[0] : 0;
      const int q_first_row = bottom ? split[1] : 0;
      const int q_cols = ( right ? map_cells[0] : split[0] ) - q_first_col;
      const int q_rows = ( bottom ? map_cells[1] : split[1] ) - q_first_row;

      // as many clusters as fit, the block of clusters centred in the quadrant
      const int clusters_x = std::max( 0, ( q_cols + external_padding ) / cluster_stride );
      const int clusters_y = std::max( 0, ( q_rows + external_padding ) / cluster_stride );
      if ( clusters_x == 0 or clusters_y == 0 )
      {
        SPDLOG_WARN( "Grave clusters: quadrant {} ({}x{} cells) is too small for a {}x{} cell cluster", quadrant, q_cols, q_rows, cluster_cells,
                     cluster_cells );
        continue;
      }
      const int offset_x = ( q_cols - ( ( clusters_x * cluster_stride ) - external_padding ) ) / 2;
      const int offset_y = ( q_rows - ( ( clusters_y * cluster_stride ) - external_padding ) ) / 2;

      std::size_t quadrant_graves = 0;
      for ( int cy = 0; cy < clusters_y; ++cy )
      {
        for ( int cx = 0; cx < clusters_x; ++cx )
        {
          const int cluster_col = q_first_col + offset_x + ( cx * cluster_stride );
          const int cluster_row = q_first_row + offset_y + ( cy * cluster_stride );

          // internal padding: every cluster cell between the slots, kept for the plants
          for ( int ly = 0; ly < cluster_cells; ++ly )
          {
            for ( int lx = 0; lx < cluster_cells; ++lx )
            {
              if ( ( lx % slot_stride ) < kGraveSlotCells and ( ly % slot_stride ) < kGraveSlotCells ) continue;
              m_grave_cluster_padding.at( quadrant ).push_back( cell_px( cluster_col + lx, cluster_row + ly ) );
            }
          }

          for ( int sy = 0; sy < slots_per_side; ++sy )
          {
            for ( int sx = 0; sx < slots_per_side; ++sx )
            {
              const int slot_col = cluster_col + ( sx * slot_stride );
              const int slot_row = cluster_row + ( sy * slot_stride );
              const sf::Vector2f slot_pos = cell_px( slot_col, slot_row );

              const auto &spritesheet = Sys::SpriteStore::instance().get_random( grave_meta_types );
              const auto grave_grid_size = spritesheet.get_grid_size();

              // omit the grave where the cluster overlaps walls, paths, buildings or the edge of the open ground
              if ( not scene_data.is_open_footprint( slot_pos, sf::Vector2u( grave_grid_size ) ) ) continue;
              bool footprint_taken = false;
              for ( int gy = 0; gy < static_cast<int>( grave_grid_size.y ) and not footprint_taken; ++gy )
              {
                for ( int gx = 0; gx < static_cast<int>( grave_grid_size.x ) and not footprint_taken; ++gx )
                {
                  footprint_taken = is_cell_taken( Cmp::Position( cell_px( slot_col + gx, slot_row + gy ), Constants::kGridSizePxF ) );
                }
              }
              if ( footprint_taken ) continue;
              if ( has_structure_collision( Cmp::RectBounds::scaled( slot_pos, spritesheet.sprite_size(), 1.f ) ) ) continue;

              const auto index = spritesheet.get_random_texture_index();
              const Cmp::Position grave_pos( slot_pos, spritesheet.sprite_size() );
              auto [mb_entt, _] = Factory::Multiblock::add_multiblock_with_segments<Cmp::Grave::MultiBlock, Cmp::Grave::Segment>(
                  reg(), slot_pos, spritesheet, index, 0, m_reserved_sm.get(), m_world_tile_sm.lock().get() );
              m_reserved_sm->insert( mb_entt, grave_pos );
              // fix the roll at creation; the consequence is resolved against the player's current luck when needed
              auto &consequence = reg().emplace_or_replace<Cmp::Grave::Consequence>( mb_entt, Cmp::RandomInt( 0, 100 ).gen(),
                                                                                     min_blood_chance_pct, consequence_line );
              // simulate the rolls now to get an idea of the distribution with game start player luck
              grave_consequences_distribution[consequence.roll( Utils::Player::get_stats( reg() ).luck() )]++;
              quadrant_graves++;
            }
          }
        }
      }
      SPDLOG_INFO( "Grave clusters: quadrant {} has {}x{} clusters of {}x{} slots, {} graves placed", quadrant, clusters_x, clusters_y, slots_per_side,
                   slots_per_side, quadrant_graves );
      placed_num_graves += quadrant_graves;
    }
    SPDLOG_INFO( "Added {} graves", placed_num_graves );
    SPDLOG_INFO( "Grave Consequences dist at current luck: {}", grave_consequences_distribution );
  }
}

template <typename MULTIBLOCK, typename MBSEGMENT>
entt::entity LevelGenerator::spawn_multiblock_at( sf::Vector2f pos, const Sprites::SpriteSheet &ss )
{
  auto [mb_entt, _] = Factory::Multiblock::add_multiblock_with_segments<MULTIBLOCK, MBSEGMENT>( reg(), pos, ss, 0, 0, m_reserved_sm.get(),
                                                                                               m_world_tile_sm.lock().get() );
  m_reserved_sm->insert( mb_entt, Cmp::Position( pos, Constants::kGridSizePxF ) );
  SPDLOG_INFO( "Added {} to {},{}", ss.type(), pos.x, pos.y );
  return mb_entt;
}

bool LevelGenerator::has_structure_collision( const Cmp::RectBounds &hitbox )
{
  using Utils::Collision::any_intersects;
  return any_intersects<Cmp::Wall>( reg(), hitbox ) or any_intersects<Cmp::Grave::Segment>( reg(), hitbox ) or
         any_intersects<Cmp::Altar::Segment>( reg(), hitbox ) or any_intersects<Cmp::Crypt::BuildingSegment>( reg(), hitbox ) or
         any_intersects<Cmp::HealingSpringBuildingSegment>( reg(), hitbox ) or any_intersects<Cmp::Ruin::BuildingSegment>( reg(), hitbox ) or
         any_intersects<Cmp::Crypt::ObjectiveSegment>( reg(), hitbox ) or any_intersects<Cmp::Grave::ExitSegment>( reg(), hitbox ) or
         any_intersects<Cmp::SpawnArea>( reg(), hitbox ) or any_intersects<Cmp::Player::Character>( reg(), hitbox );
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

    auto lo_sprite_size = Sys::SpriteStore::instance().get( ms.type() ).sprite_size();
    auto new_lo_hitbox = Cmp::RectBounds::scaled( random_pos.position, lo_sprite_size, 1.f );

    // Check collisions with walls, graves, shrines, the exit, and anything else already claiming this position
    if ( not has_structure_collision( new_lo_hitbox ) and not is_area_taken( new_lo_hitbox.getBounds() ) )
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

    if ( not is_cell_taken( random_origin_position ) )
    {
      float zorder = Sys::SpriteStore::instance().get( "sprite.graveyard.pots" ).sprite_size().y;

      Cmp::RandomInt pot_picker( 0, 2 );
      Factory::Loot::create_loot_container( reg(), random_entity, random_origin_position, "sprite.graveyard.pots", pot_picker.gen(), zorder );
      assigned_entts.push_back( random_entity );
      m_claims_sm.insert( random_entity, random_origin_position );
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

    if ( not is_cell_taken( random_origin_position ) )
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

std::vector<entt::entity> LevelGenerator::gen_random_plants( const Scene::SceneData &scene_data )
{
  std::vector<entt::entity> assigned_entts;

  // Every plant item with a non-zero items.json "spawn_count_per_quadrant" (a per-quadrant quota), tracking how many of it were
  // placed in the current quadrant against that quota
  struct PlantQuota
  {
    int placed{ 0 };
    int quota{ 0 };
  };
  std::unordered_map<Sys::ItemKey, PlantQuota> plant_tracker;
  for ( const auto &item_key : Sys::ItemStore::instance().get_all() )
  {
    if ( not item_key.str().starts_with( "item.plant." ) ) continue;
    const int quota = Sys::ItemStore::instance().get( item_key ).spawn_count_per_quadrant();
    if ( quota > 0 ) plant_tracker.emplace( item_key, PlantQuota{ .placed = 0, .quota = quota } );
  }
  if ( plant_tracker.empty() )
  {
    SPDLOG_INFO( "gen_random_plants: every plant has a spawn_count_per_quadrant of 0, no plants generated" );
    return assigned_entts;
  }

  // helpers
  const auto below_quota = [&]( const auto &kv ) { return kv.second.placed < kv.second.quota; };
  const auto quota_reached = [&] { return std::ranges::none_of( plant_tracker, below_quota ); };

  // Plants only grow in the grave clusters' internal padding (see gen_graveyard_exterior_multiblocks())
  std::unordered_set<std::int64_t> padding_cells;
  for ( const auto &quadrant_padding : m_grave_cluster_padding )
  {
    for ( const auto &pos : quadrant_padding )
    {
      padding_cells.insert( cell_key( pos ) );
    }
  }
  if ( padding_cells.empty() ) { SPDLOG_WARN( "gen_random_plants: no grave cluster padding cells, no plants generated" ); }

  // Snapshot every candidate position once, bucketed by map quadrant, and visit each quadrant's candidates in
  // random order, instead of re-scanning the registry for a random position on every attempt. Each tile is tried at most once.
  using Candidate = std::pair<entt::entity, Cmp::Position>;
  std::array<std::vector<Candidate>, 4> candidates_by_quadrant;
  auto candidate_view = reg().view<Cmp::Position>( entt::exclude<Cmp::Player::Character, Cmp::Obstacle> );
  for ( auto [tile_entt, pos_cmp] : candidate_view.each() )
  {
    if ( not padding_cells.contains( cell_key( pos_cmp.position ) ) ) continue;
    candidates_by_quadrant.at( scene_data.quadrant_of( pos_cmp.position ) ).emplace_back( tile_entt, pos_cmp );
  }
  Cmp::RandomInt shuffle_rng( 0, 1 );

  std::vector<decltype( plant_tracker )::iterator> plant_types_below_quota;
  for ( std::size_t quadrant = 0; quadrant < candidates_by_quadrant.size(); ++quadrant )
  {
    auto &candidates = candidates_by_quadrant.at( quadrant );
    std::ranges::shuffle( candidates, shuffle_rng );
    for ( auto &[_, tracked] : plant_tracker )
    {
      tracked.placed = 0;
    }

    for ( const auto &[random_entity, random_pos] : candidates )
    {
      if ( quota_reached() ) break;

      // cheap O(1) rejection before doing any plant work - PlantSystem re-checks the whole footprint against structures
      if ( is_cell_taken( random_pos ) ) continue;

      // Build a list of plant types that are still short of the quota and pick one at random
      plant_types_below_quota.clear();
      for ( auto it = plant_tracker.begin(); it != plant_tracker.end(); ++it )
      {
        if ( below_quota( *it ) ) plant_types_below_quota.push_back( it );
      }
      auto selected_plant_iter = plant_types_below_quota.at( Cmp::RandomInt( 0, static_cast<int>( plant_types_below_quota.size() - 1 ) ).gen() );

      const Sys::ItemKey &chosen_plant_item_type = selected_plant_iter->first;

      // PlantSystem only knows about structures, so reject footprints that overlap this pass's claims
      // (e.g. a loot container under the top cell of a 1x2 plant) here
      const auto &plant_sprite_type = Sys::ItemStore::instance().get( chosen_plant_item_type ).sprite_type;
      const auto plant_grid_size = Sys::SpriteStore::instance().get( plant_sprite_type ).get_grid_size();
      bool footprint_claimed = false;
      for ( int gy = 0; gy < static_cast<int>( plant_grid_size.y ) and not footprint_claimed; ++gy )
      {
        for ( int gx = 0; gx < static_cast<int>( plant_grid_size.x ) and not footprint_claimed; ++gx )
        {
          const sf::Vector2f offset( static_cast<float>( gx ) * Constants::kGridSizePxF.x, static_cast<float>( gy ) * Constants::kGridSizePxF.y );
          footprint_claimed = is_cell_taken( Cmp::Position( random_pos.position + offset, Constants::kGridSizePxF ) );
        }
      }
      if ( footprint_claimed ) continue;

      // Create the plant in the game area and increment the tracker counter
      bool created = false;
      get_systems_event_queue().trigger(
          Events::CreatePlantEvent( chosen_plant_item_type, random_pos, [&created]( [[maybe_unused]] entt::entity plant_entt ) { created = true; } ) );
      if ( created )
      {
        assigned_entts.push_back( random_entity );
        ++selected_plant_iter->second.placed;
      }
    }

    if ( not quota_reached() )
    {
      SPDLOG_WARN( "gen_random_plants: quadrant {} ran out of free positions ({} candidates) before every plant type reached its spawn_count_per_quadrant",
                   quadrant, candidates.size() );
    }
    for ( const auto &[plant_type, tracked] : plant_tracker )
    {
      SPDLOG_INFO( "plant_tracker: quadrant {} {} placed {} of {}", quadrant, plant_type, tracked.placed, tracked.quota );
    }
  }
  return assigned_entts;
}

void LevelGenerator::init( const PathFinding::SpatialHashGridSharedPtr &reserved_sm, const PathFinding::SpatialHashGridSharedPtr &item_sm )
{
  m_obstacle_sm->clear();
  m_void_sm->clear();
  m_non_obstacle_sm->clear();

  m_reserved_sm = reserved_sm;
  m_reserved_sm->clear();
  m_claims_sm.clear();
  m_item_sm = item_sm;
  m_world_tile_sm.reset();
  for ( auto &padding : m_grave_cluster_padding )
  {
    padding.clear();
  }
}

} // namespace Game::Sys::ProcGen