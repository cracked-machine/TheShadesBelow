#include <Components/Random.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/BaseSystem.hpp>
#include <Utils/Constants.hpp>
#include <utility>

#include <spdlog/spdlog.h>

namespace Game::Sprites
{

SpriteSheet::SpriteSheet( Sys::SpriteKey type, std::string display_name, const std::vector<float> &zorder_list,
                          const std::filesystem::path &spritesheet_png, const std::vector<uint32_t> &spritesheet_selections, sf::Vector2i grid_size,
                          unsigned int indices_per_frame, unsigned int indices_per_sequence, std::vector<bool> collision_mask,
                          sf::Vector2i door_position )
    : m_sprite_type{ std::move( type ) },
      m_display_name( std::move( display_name ) ),
      m_zorder_list( zorder_list ),
      m_grid_size{ grid_size },
      m_indices_per_frame{ indices_per_frame },
      m_indices_per_sequence{ indices_per_sequence },
      m_collision_mask{ normalise_collision_mask( std::move( collision_mask ), grid_size, m_sprite_type ) },
      m_door_position( door_position )
{
  m_spritesheet_texture = std::make_unique<sf::Texture>();
  if ( !m_spritesheet_texture->loadFromFile( spritesheet_png ) )
  {
    SPDLOG_ERROR( "Unable to load spritesheet texture {}", spritesheet_png.string() );
    throw std::runtime_error( "Unable to load spritesheet texture: " + spritesheet_png.string() );
  }
  m_spritesheet_texture->setSmooth( false );
  SPDLOG_DEBUG( "Loaded spritesheet texture: {}", tilemap_path.string() );
  if ( !add_sprite( spritesheet_selections ) )
  {
    SPDLOG_CRITICAL( "Failed to add sprite: {}", spritesheet_png.string() );
    throw std::runtime_error( "Failed to add sprite: " + spritesheet_png.string() );
  }
}

SpriteSheet::SpriteSheet( Sys::SpriteKey type, std::string display_name, const std::vector<float> &zorder_list, sf::Texture tilemap_texture,
                          const std::vector<uint32_t> &tilemap_picks, sf::Vector2i grid_size, unsigned int indices_per_frame,
                          unsigned int indices_per_sequence, std::vector<bool> collision_mask, sf::Vector2i door_position )
    : m_sprite_type{ std::move( type ) },
      m_display_name( std::move( display_name ) ),
      m_zorder_list( zorder_list ),
      m_grid_size{ grid_size },
      m_indices_per_frame{ indices_per_frame },
      m_indices_per_sequence{ indices_per_sequence },
      m_collision_mask{ normalise_collision_mask( std::move( collision_mask ), grid_size, m_sprite_type ) },
      m_door_position( door_position )
{
  SPDLOG_DEBUG( "Loaded tilemap texture" );
  m_spritesheet_texture = std::make_shared<sf::Texture>( std::move( tilemap_texture ) );
  m_spritesheet_texture->setSmooth( false );
  if ( !add_sprite( tilemap_picks ) )
  {
    SPDLOG_CRITICAL( "Failed to load tilemap" );
    throw std::runtime_error( "Failed to load tilemap" );
  }
}

std::vector<bool> SpriteSheet::normalise_collision_mask( std::vector<bool> collision_mask, sf::Vector2i grid_size, const Sys::SpriteKey &type )
{
  const auto cell_count = static_cast<std::size_t>( grid_size.x * grid_size.y );

  // omitted: every cell collides. Readers treat an empty mask that way.
  if ( collision_mask.empty() ) return collision_mask;

  // shortcut: a single entry applies to every cell
  if ( collision_mask.size() == 1 )
  {
    collision_mask.assign( cell_count, collision_mask.front() );
    return collision_mask;
  }

  if ( collision_mask.size() < cell_count )
  {
    SPDLOG_ERROR( "{} collision_mask has {} entries, expected 1 or at least {}", type.str(), collision_mask.size(), cell_count );
    throw std::runtime_error( "Invalid collision_mask for " + type.str() + ": expected 1 or at least " + std::to_string( cell_count ) +
                              " entries, got " + std::to_string( collision_mask.size() ) );
  }
  return collision_mask;
}

bool SpriteSheet::add_sprite( const std::vector<uint32_t> &tilemap_picks )
{
  if ( tilemap_picks.empty() )
  {
    SPDLOG_WARN( "Empty tilemap_picks provided" );
    return false;
  }

  SPDLOG_DEBUG( "{} requested {} tiles", m_sprite_type, tilemap_picks.size() );
  for ( const auto &tile_idx : tilemap_picks )
  {
    sf::Vector2u kGridSquareSizePixels{ Constants::kGridSizePx.x * m_grid_size.x, Constants::kGridSizePx.y * m_grid_size.y };

    sf::VertexArray current_va( sf::PrimitiveType::Triangles, 6 );

    // Calculate texture coordinates based on 16x16 base tile grid (not sprite grid)
    const int base_tiles_per_row = m_spritesheet_texture->getSize().x / Constants::kGridSizePx.x;
    const int base_tile_x = tile_idx % base_tiles_per_row;
    const int base_tile_y = tile_idx / base_tiles_per_row;

    const int tu = base_tile_x * Constants::kGridSizePx.x;
    const int tv = base_tile_y * Constants::kGridSizePx.y;

    // draw the two triangles within local space using the sprite dimensions
    current_va[0].position = sf::Vector2f( 0, 0 );
    current_va[1].position = sf::Vector2f( kGridSquareSizePixels.x, 0 );
    current_va[2].position = sf::Vector2f( 0, kGridSquareSizePixels.y );
    current_va[3].position = sf::Vector2f( 0, kGridSquareSizePixels.y );
    current_va[4].position = sf::Vector2f( kGridSquareSizePixels.x, 0 );
    current_va[5].position = sf::Vector2f( kGridSquareSizePixels.x, kGridSquareSizePixels.y );

    current_va[0].texCoords = sf::Vector2f( tu, tv );
    current_va[1].texCoords = sf::Vector2f( tu + kGridSquareSizePixels.x, tv );
    current_va[2].texCoords = sf::Vector2f( tu, tv + kGridSquareSizePixels.y );
    current_va[3].texCoords = sf::Vector2f( tu, tv + kGridSquareSizePixels.y );
    current_va[4].texCoords = sf::Vector2f( tu + kGridSquareSizePixels.x, tv );
    current_va[5].texCoords = sf::Vector2f( tu + kGridSquareSizePixels.x, tv + kGridSquareSizePixels.y );

    SPDLOG_DEBUG( "Added sprite index {} at texture coords ({}, {})", tile_idx, tu, tv );
    SPDLOG_DEBUG( "Sprite vertex positions: [({}, {}) to  ({}, {})]", current_va[0].position.x, current_va[0].position.y, current_va[5].position.x,
                  current_va[5].position.y );

    m_va_list.push_back( current_va );
  }
  SPDLOG_DEBUG( "Created {} sprites ", m_va_list.size() );
  return true;
}

std::size_t SpriteSheet::get_random_texture_index() const
{
  return static_cast<std::size_t>( Cmp::RandomInt( 0, static_cast<int>( sheet_size() ) - 1 ).gen() );
}

} // namespace Game::Sprites