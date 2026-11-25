#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SceneControl/SceneData.hpp>
#include <Sprites/VertexFloor.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Graphics/Color.hpp>
#include <entt/entity/registry.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace Game::Sprites::Containers
{

void VertexFloor::draw( sf::RenderTarget &target, sf::RenderStates states ) const
{
  SPDLOG_DEBUG( "TileMap::draw - instance: {}, vertices: {}", (void *)this, m_vertices.getVertexCount() );
  if ( m_vertices.getVertexCount() == 0 )
  {
    SPDLOG_WARN( "Attempting to draw empty tilemap" );
    return;
  }

  // apply the transform
  states.transform *= getTransform();

  // apply the tileset texture
  states.texture = &m_tileset;

  // draw the vertex array
  target.draw( m_vertices, states );
}

void VertexFloor::create( const PathFinding::SpatialHashGrid &void_sm, const Scene::SceneMapSharedPtr &sc )
{
  if ( not sc ) throw std::runtime_error( "Scene::SceneConfigSharedPtr is not initialised" );

  // load the tilemap from the scene configuration object
  try
  {
    if ( !std::filesystem::exists( sc->floor_tileset_image() ) )
    {
      SPDLOG_CRITICAL( "Texture file does not exist: {}", sc->floor_tileset_image().string() );
      throw std::runtime_error( "Texture file not found" );
    }
    if ( !m_tileset.loadFromFile( sc->floor_tileset_image() ) )
    {
      SPDLOG_CRITICAL( "Unable to load tileset texture file: {}", sc->floor_tileset_image().string() );
      throw std::runtime_error( "Failed to load texture" );
    }

  } catch ( const std::exception &e )
  {
    SPDLOG_CRITICAL( "Failed to initialize TileMap: {}", e.what() );
    throw;
  }

  // Validate texture is loaded before creating vertices
  if ( m_tileset.getSize().x == 0 || m_tileset.getSize().y == 0 )
  {
    SPDLOG_ERROR( "Tileset texture not loaded or invalid" );
    throw;
  }
  const auto tile_size = Constants::kGridSizePx;
  const sf::Vector2u texture_size = m_tileset.getSize();
  const unsigned int tiles_per_row = texture_size.x / tile_size.x;

  Cmp::RandomInt floortile_picker{ 0, static_cast<int>( sc->floor_tileset_pool().size() - 1 ) };

  // resize the vertex array for squares made of two triangles
  m_vertices.setPrimitiveType( sf::PrimitiveType::Triangles );

  auto [map_size_grid, map_size_pixel] = sc->map_size();

  SPDLOG_DEBUG( "Generating tilemap for {}x{}", map_size_grid.x, map_size_grid.y );
  for ( unsigned int w = 0; w < map_size_grid.x; w++ )
  {
    for ( unsigned int h = 0; h < map_size_grid.y; h++ )
    {
      auto pick = floortile_picker.gen();
      SPDLOG_DEBUG( "Chosen tile idx {}", pick );
      const unsigned int tile_number = sc->floor_tileset_pool()[pick];

      // check if we have anything in the void spatial map for this position
      Cmp::Position lookup_position( { w * Constants::kGridSizePxF.x, h * Constants::kGridSizePxF.y }, Constants::kGridSizePxF );

      auto results = void_sm.at( lookup_position );
      if ( not results.empty() ) continue;

      // find its position in the tileset texture
      const unsigned int tu = tile_number % tiles_per_row;
      const unsigned int tv = tile_number / tiles_per_row;

      // Cache position calculations
      const auto left = static_cast<float>( w * tile_size.x );
      const auto top = static_cast<float>( h * tile_size.y );
      const auto right = static_cast<float>( ( w + 1 ) * tile_size.x );
      const auto bottom = static_cast<float>( ( h + 1 ) * tile_size.y );

      const auto tex_left = static_cast<float>( tu * tile_size.x );
      const auto tex_top = static_cast<float>( tv * tile_size.y );
      const auto tex_right = static_cast<float>( ( tu + 1 ) * tile_size.x );
      const auto tex_bottom = static_cast<float>( ( tv + 1 ) * tile_size.y );

      // define the 6 corners of the two triangles (counter-clockwise)
      m_vertices.append( { .position = { left, top }, .color = sf::Color::White, .texCoords = { tex_left, tex_top } } );
      m_vertices.append( { .position = { right, top }, .color = sf::Color::White, .texCoords = { tex_right, tex_top } } );
      m_vertices.append( { .position = { left, bottom }, .color = sf::Color::White, .texCoords = { tex_left, tex_bottom } } );
      m_vertices.append( { .position = { left, bottom }, .color = sf::Color::White, .texCoords = { tex_left, tex_bottom } } );
      m_vertices.append( { .position = { right, top }, .color = sf::Color::White, .texCoords = { tex_right, tex_top } } );
      m_vertices.append( { .position = { right, bottom }, .color = sf::Color::White, .texCoords = { tex_right, tex_bottom } } );
    }
  }

  SPDLOG_DEBUG( "Created tilemap: {} vertices", m_vertices.getVertexCount() );
}

void VertexFloor::remove( sf::Vector2f pos )
{
  // Snap to grid to ensure position matches tilemap vertex positions
  pos = Utils::snap_to_grid( pos );

  // determine the vertices to remove
  const auto left = static_cast<float>( pos.x );
  const auto top = static_cast<float>( pos.y );
  const auto right = static_cast<float>( pos.x + Constants::kGridSizePxF.x );
  const auto bottom = static_cast<float>( pos.y + Constants::kGridSizePxF.y );

  // clang-format off
  const std::array<sf::Vector2f, 4> tile_corners = {
    sf::Vector2f{ left, top },
    sf::Vector2f{ right, top },
    sf::Vector2f{ left, bottom },
    sf::Vector2f{ right, bottom }
  };
  // clang-format on

  auto is_tile_vertex = [&]( const sf::Vertex &v )
  {
    for ( const auto &corner : tile_corners )
    {
      if ( v.position == corner ) return true;
    }
    return false;
  };

  // Tiles are stored as groups of 6 vertices; skip any group where the first vertex matches the tile
  sf::VertexArray new_vertices( sf::PrimitiveType::Triangles );
  const auto count = m_vertices.getVertexCount();
  for ( size_t i = 0; i + 5 < count; i += 6 )
  {
    if ( is_tile_vertex( m_vertices[i] ) && is_tile_vertex( m_vertices[i + 1] ) && is_tile_vertex( m_vertices[i + 2] ) )
    {
      SPDLOG_DEBUG( "sinkhole collided with tile at {},{}", pos.x, pos.y );
      continue; // skip this tile's 6 vertices
    }
    for ( size_t j = i; j < i + 6; ++j )
    {
      new_vertices.append( m_vertices[j] );
    }
  }
  // Move the vertices that dont collide into the new `m_vertices`
  m_vertices = std::move( new_vertices );
}

} // namespace Game::Sprites::Containers