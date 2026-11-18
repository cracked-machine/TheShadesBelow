#include <SceneControl/SceneData.hpp>
#include <Utils/Constants.hpp>

#include <exception>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace Game::Scene
{

SceneData::SceneData( const std::filesystem::path &map_file ) { deserialize( map_file ); }

void SceneData::deserialize( const std::filesystem::path &json_scene_file_path )
{
  nlohmann::json scene_tilemap_json = load_json_file( json_scene_file_path );

  bool found_floor_tileset = false;
  bool found_wall_tileset = false;
  bool found_path_tileset = false;
  bool found_main_tileset = false;

  deserialize_tilelayers( scene_tilemap_json );
  for ( const auto &tileset : scene_tilemap_json.at( "tilesets" ) )
  {
    // embedded tilesets
    if ( not found_floor_tileset ) { found_floor_tileset = deserialize_int_floor_tileset( tileset ); }
    if ( not found_wall_tileset ) { found_wall_tileset = deserialize_int_wall_tileset( tileset ); }
    if ( not found_path_tileset ) { found_path_tileset = deserialize_int_path_tileset( tileset ); }

    // external 'main' tileset metadata
    if ( not found_main_tileset ) { found_main_tileset = deserialize_ext_main_tileset( json_scene_file_path, tileset ); }
  }
  if ( not found_floor_tileset ) { throw std::runtime_error( "JSON Error: missing floor tileset: " + json_scene_file_path.string() ); }
  if ( not found_main_tileset ) { throw std::runtime_error( "JSON Error: missing main tileset: " + json_scene_file_path.string() ); }
  if ( not found_wall_tileset ) { throw std::runtime_error( "JSON Error: missing wall tileset: " + json_scene_file_path.string() ); }

  retrieve_player_start_pos();
}

bool SceneData::deserialize_int_floor_tileset( const nlohmann::json &json )
{
  if ( not json.contains( "name" ) or json.at( "name" ).get<std::string>() != "floor" ) return false;
  if ( not json.contains( "image" ) ) return false;

  std::string image = get_string( json, "image" );
  m_map_data.floor_tileset.tileset_image = get_abs_path( image );

  m_map_data.floor_tileset.tileset_pool.clear();
  m_map_data.floor_tileset.tileset_pool = get_int_list_property( json, "pool" );
  if ( m_map_data.floor_tileset.tileset_pool.empty() ) throw std::runtime_error( "Floor tileset 'pool' property is missing or empty" );
  m_map_data.floor_tileset.tile_size = sf::Vector2u( get_int( json, "tilewidth" ), get_int( json, "tileheight" ) );

  return true;
}

bool SceneData::deserialize_int_wall_tileset( const nlohmann::json &json )
{

  if ( not json.contains( "class" ) or json.at( "class" ).get<std::string>() != "wall" ) return false;
  m_map_data.wall_tileset.first_gid = get_int( json, "firstgid" );
  m_map_data.wall_tileset.name = get_string( json, "name" );
  m_map_data.wall_tileset.tile_size = sf::Vector2u( get_int( json, "tilewidth" ), get_int( json, "tileheight" ) );
  return true;
}

bool SceneData::deserialize_int_path_tileset( const nlohmann::json &json )
{

  if ( not json.contains( "class" ) or json.at( "class" ).get<std::string>() != "path" ) return false;
  m_map_data.path_tileset.first_gid = get_int( json, "firstgid" );
  m_map_data.path_tileset.name = get_string( json, "name" );
  m_map_data.path_tileset.tile_size = sf::Vector2u( get_int( json, "tilewidth" ), get_int( json, "tileheight" ) );
  return true;
}

bool SceneData::deserialize_ext_main_tileset( const std::filesystem::path &scene_tilemap_path, const nlohmann::json &tileset )
{
  std::filesystem::path main_tileset_path;
  if ( not tileset.contains( "source" ) or not tileset.contains( "firstgid" ) ) return false;

  std::filesystem::path source = get_string( tileset, "source" );

  // We only expect one external tileset so just return early if others are found
  if ( source.filename() != "main.json" ) return false;

  main_tileset_path = Constants::res_dir / "scenes" / source;
  m_map_data.main_tileset.first_gid = tileset.at( "firstgid" ).get<int>();

  if ( main_tileset_path.empty() ) { throw std::runtime_error( "Cannot find file 'main.json' from " + scene_tilemap_path.string() ); }

  // now deserialize the external main tileset json contents
  try
  {
    nlohmann::json main_tileset_json = load_json_file( main_tileset_path );

    if ( not main_tileset_json.contains( "tiles" ) ) throw std::runtime_error( "Missing JSON property 'tiles' in main tileset." );

    std::set<std::string> found_types;
    for ( const auto &tile : main_tileset_json.at( "tiles" ) )
    {
      auto id = get_int( tile, "id" );
      auto type = get_string( tile, "type" );

      // clang-format off
      if      ( type == "void"     ) { m_map_data.main_tileset.void_tile_id     = id; }
      else if ( type == "wall"     ) { m_map_data.main_tileset.wall_tile_id     = id; }
      else if ( type == "open"     ) { m_map_data.main_tileset.open_tile_id     = id; }
      else if ( type == "spawn"    ) { m_map_data.main_tileset.spawn_tile_id    = id; }
      else if ( type == "player"   ) { m_map_data.main_tileset.player_tile_id   = id; }
      else if ( type == "exit"     ) { m_map_data.main_tileset.exit_tile_id     = id; }
      else if ( type == "reserved" ) { m_map_data.main_tileset.reserved_tile_id = id; }
      else if ( type == "path"     ) { m_map_data.main_tileset.path_tile_id     = id; }
      else { SPDLOG_WARN( "Unknown 'type' found in main tileset: {}", type ); }
      // clang-format on

      found_types.insert( type );
    }

    for ( const std::string required : { "void", "wall", "open", "spawn", "player", "exit", "reserved", "path" } )
    {
      if ( not found_types.contains( required ) ) throw std::runtime_error( "Missing required tile type '" + required + "' in main tileset" );
    }
  } catch ( std::exception &e )
  {
    throw std::runtime_error( "Error in main tileset '" + main_tileset_path.string() + "': " + e.what() );
  }

  return true;
}

void SceneData::deserialize_tilelayers( const nlohmann::json &json )
{
  //! @brief Misc layers
  auto get_list = [&]<typename T>( const nlohmann::json &j, const std::string &key, std::vector<T> &out )
  {
    if ( not j.contains( key ) ) throw std::runtime_error( "Missing JSON property '" + key + "'" );
    try
    {
      out = j.at( key ).get<std::vector<T>>();
    } catch ( const nlohmann::json::type_error &e )
    {
      throw std::runtime_error( "JSON Error: '" + key + "': " + e.what() );
    }
  };

  //! @brief Solid layer
  auto get_solid_list = [&]( const nlohmann::json &j, std::vector<sf::FloatRect> &out )
  {
    if ( not j.contains( "objects" ) ) throw std::runtime_error( "Missing JSON property objects in solid object layer" );
    try
    {
      for ( const auto &object : j.at( "objects" ) )
      {
        if ( not object.contains( "name" ) or object.at( "name" ).get<std::string>() != "solid" ) continue;
        sf::FloatRect r( { get_float( object, "x" ), get_float( object, "y" ) }, { get_float( object, "width" ), get_float( object, "height" ) } );
        out.push_back( r );
      }
    } catch ( const nlohmann::json::type_error &e )
    {
      throw std::runtime_error( std::string( "JSON Error: object layer - " ) + e.what() );
    }
  };

  //! @brief Multiblock layer
  auto get_multiblock_list = [&]( const nlohmann::json &j, std::multimap<std::string, sf::Vector2f> &out )
  {
    if ( not j.contains( "objects" ) ) throw std::runtime_error( "Missing JSON property 'objects' in multiblock layer" );
    try
    {
      for ( const auto &object : j.at( "objects" ) )
      {
        if ( not object.contains( "name" ) ) throw std::runtime_error( "Missing JSON property 'name' in multiblock object layer" );
        std::string type = object.at( "name" ).get<std::string>();
        sf::Vector2f pos( get_float( object, "x" ), get_float( object, "y" ) );
        out.insert( { type, pos } );
      }
    } catch ( const nlohmann::json::type_error &e )
    {
      throw std::runtime_error( std::string( "JSON Error: object layer - " ) + e.what() );
    }
  };

  //! @brief Multiblock layer
  auto get_developer_list = [&]( const nlohmann::json &j, std::multimap<std::string, sf::Vector2f> &out )
  {
    if ( not j.contains( "objects" ) ) throw std::runtime_error( "Missing JSON property 'objects' in developer layer" );
    try
    {
      for ( const auto &object : j.at( "objects" ) )
      {
        if ( not object.contains( "name" ) ) throw std::runtime_error( "Missing JSON property 'name' in developer object layer" );
        std::string type = object.at( "name" ).get<std::string>();
        sf::Vector2f pos( get_float( object, "x" ), get_float( object, "y" ) );
        out.insert( { type, pos } );
      }
    } catch ( const nlohmann::json::type_error &e )
    {
      throw std::runtime_error( std::string( "JSON Error: object layer - " ) + e.what() );
    }
  };

  //! @brief POI layer - point markers, only the position is used
  auto get_poi_list = [&]( const nlohmann::json &j, std::vector<sf::Vector2f> &out )
  {
    if ( not j.contains( "objects" ) ) throw std::runtime_error( "Missing JSON property 'objects' in poi layer" );
    try
    {
      for ( const auto &object : j.at( "objects" ) )
      {
        out.emplace_back( get_float( object, "x" ), get_float( object, "y" ) );
      }
    } catch ( const nlohmann::json::type_error &e )
    {
      throw std::runtime_error( std::string( "JSON Error: poi layer - " ) + e.what() );
    }
  };

  m_map_data.map_size = sf::Vector2u( get_int( json, "width" ), get_int( json, "height" ) );

  if ( json.contains( "layers" ) )
  {
    for ( const auto &layer : json.at( "layers" ) )
    {
      if ( not layer.contains( "name" ) ) throw std::runtime_error( "Missing JSON property 'name' in layer" );
      if ( layer.at( "name" ).get<std::string>() == "levelgen" ) { get_list( layer, "data", m_map_data.levelgen_tilelayer ); }
      if ( layer.at( "name" ).get<std::string>() == "player" ) { get_list( layer, "data", m_map_data.player_tilelayer ); }
      if ( layer.at( "name" ).get<std::string>() == "wall" ) { get_list( layer, "data", m_map_data.wall_tilelayer ); }
      if ( layer.at( "name" ).get<std::string>() == "path" ) { get_list( layer, "data", m_map_data.path_tilelayer ); }
      if ( layer.at( "name" ).get<std::string>() == "solid" ) { get_solid_list( layer, m_map_data.solid_objectlayer ); }
      if ( layer.at( "name" ).get<std::string>() == "multiblock" ) { get_multiblock_list( layer, m_map_data.multiblock_objectlayer ); }
      if ( layer.at( "name" ).get<std::string>() == "developer" ) { get_developer_list( layer, m_map_data.developer_objectlayer ); }
      if ( layer.at( "name" ).get<std::string>() == "poi" ) { get_poi_list( layer, m_map_data.poi_objectlayer ); }
    }
    if ( m_map_data.levelgen_tilelayer.empty() ) { throw std::runtime_error( "Missing JSON property tilelayer: 'levelgen'" ); }
  }
}

void SceneData::retrieve_player_start_pos()
{
  bool found_player_start_position = false;
  for ( auto [i, tile] : std::views::enumerate( m_map_data.player_tilelayer ) )
  {
    if ( m_map_data.main_tileset.player_tile_id == tile - m_map_data.main_tileset.first_gid )
    {
      auto col = i % m_map_data.map_size.x; // wraps back to zero every 'w' tiles
      auto row = i / m_map_data.map_size.x; // increments every 'w' tiles
      sf::Vector2u result( col, row );
      m_map_data.player_start_position = result;
      found_player_start_position = true;
      break; // stop at the first player position
    }
  }
  if ( not found_player_start_position ) { SPDLOG_WARN( "No player start tile found in 'player' layer, defaulting to (0,0)" ); }
}

std::pair<sf::Vector2u, sf::Vector2f> SceneData::get_player_start_position() const
{
  // clang-format off
  return { 
    m_map_data.player_start_position, 
    sf::Vector2f{ static_cast<float>( m_map_data.player_start_position.x) * static_cast<float>(Constants::kGridSizePx.x) ,
                  static_cast<float>( m_map_data.player_start_position.y) * static_cast<float>(Constants::kGridSizePx.y)  } 
  };
  // clang-format on
}

sf::FloatRect SceneData::get_spawn_area_bounds() const
{
  bool found_spawn_tile = false;
  sf::Vector2u min_grid{ 0, 0 };
  sf::Vector2u max_grid{ 0, 0 };

  for ( auto [i, tile] : std::views::enumerate( m_map_data.levelgen_tilelayer ) )
  {
    if ( tile != spawn_tile_id() ) continue;

    auto col = static_cast<unsigned int>( i ) % m_map_data.map_size.x;
    auto row = static_cast<unsigned int>( i ) / m_map_data.map_size.x;

    if ( not found_spawn_tile )
    {
      min_grid = max_grid = { col, row };
      found_spawn_tile = true;
      continue;
    }
    min_grid.x = std::min( min_grid.x, col );
    min_grid.y = std::min( min_grid.y, row );
    max_grid.x = std::max( max_grid.x, col );
    max_grid.y = std::max( max_grid.y, row );
  }

  if ( not found_spawn_tile )
  {
    SPDLOG_WARN( "No 'spawn' tiles found in 'levelgen' layer, spawn area bounds default to zero-size" );
    return {};
  }

  return { sf::Vector2f{ static_cast<float>( min_grid.x ) * static_cast<float>( Constants::kGridSizePx.x ),
                         static_cast<float>( min_grid.y ) * static_cast<float>( Constants::kGridSizePx.y ) },
           sf::Vector2f{ static_cast<float>( max_grid.x - min_grid.x + 1 ) * static_cast<float>( Constants::kGridSizePx.x ),
                         static_cast<float>( max_grid.y - min_grid.y + 1 ) * static_cast<float>( Constants::kGridSizePx.y ) } };
}

std::array<sf::Vector2f, 4> SceneData::poi_positions_by_quadrant() const
{
  const auto &pois = m_map_data.poi_objectlayer;
  if ( pois.size() != 4 ) { throw std::runtime_error( "Expected 4 markers in 'poi' layer, found " + std::to_string( pois.size() ) ); }

  std::array<sf::Vector2f, 4> result{};
  std::array<bool, 4> found{};
  for ( const auto &pos : pois )
  {
    const auto quadrant = quadrant_of( pos );
    if ( found[quadrant] ) { throw std::runtime_error( "More than one 'poi' marker in quadrant " + std::to_string( quadrant ) ); }
    found[quadrant] = true;
    result[quadrant] = pos;
  }
  return result;
}

std::size_t SceneData::quadrant_of( sf::Vector2f pos_px ) const
{
  const auto [_, map_size_px] = map_size();
  const std::size_t right = pos_px.x >= map_size_px.x / 2.f ? 1 : 0;
  const std::size_t bottom = pos_px.y >= map_size_px.y / 2.f ? 1 : 0;
  return right + ( 2 * bottom );
}

std::vector<sf::Vector2f> SceneData::levelgen_positions( int gid ) const
{
  std::vector<sf::Vector2f> result;
  const auto w = m_map_data.map_size.x;
  for ( const auto [i, tile] : std::views::enumerate( m_map_data.levelgen_tilelayer ) )
  {
    if ( tile != gid ) continue;
    const auto col = static_cast<unsigned int>( i ) % w;
    const auto row = static_cast<unsigned int>( i ) / w;
    result.emplace_back( static_cast<float>( col ) * Constants::kGridSizePxF.x, static_cast<float>( row ) * Constants::kGridSizePxF.y );
  }
  return result;
}

bool SceneData::is_open_footprint( sf::Vector2f origin_px, sf::Vector2u grid_size ) const
{
  const auto first_col = static_cast<int>( origin_px.x / Constants::kGridSizePxF.x );
  const auto first_row = static_cast<int>( origin_px.y / Constants::kGridSizePxF.y );
  const auto w = static_cast<int>( m_map_data.map_size.x );
  const auto h = static_cast<int>( m_map_data.map_size.y );
  for ( int row = first_row; row < first_row + static_cast<int>( grid_size.y ); ++row )
  {
    for ( int col = first_col; col < first_col + static_cast<int>( grid_size.x ); ++col )
    {
      if ( col < 0 or row < 0 or col >= w or row >= h ) return false;
      if ( m_map_data.levelgen_tilelayer.at( static_cast<std::size_t>( ( row * w ) + col ) ) != open_tile_id() ) return false;
    }
  }
  return true;
}

std::pair<sf::Vector2u, sf::Vector2f> SceneData::map_size() const
{
  // clang-format off
  return { 
    m_map_data.map_size, 
    sf::Vector2f{ static_cast<float>( m_map_data.map_size.x) * static_cast<float>(Constants::kGridSizePx.x) ,
                  static_cast<float>( m_map_data.map_size.y) * static_cast<float>(Constants::kGridSizePx.y)  } 
  };
  // clang-format on
}

} // namespace Game::Scene