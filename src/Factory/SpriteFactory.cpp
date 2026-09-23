#include <Components/Random.hpp>
#include <Factory/SpriteFactory.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Utils/JsonDeserializer.hpp>

#include <SFML/Graphics/Image.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <regex>
#include <spdlog/fmt/bundled/ranges.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace Game::Sprites
{
void SpriteFactory::init()
{
  std::filesystem::path json_path( "res/json/spritesheets.json" );
  auto j = Utils::JsonDeserializer::load_json_file( json_path );

  if ( not j.contains( "sprites" ) ) throw std::runtime_error( "Missing 'sprites' from " + json_path.string() );
  const auto &sprites = j.at( "sprites" );
  for ( const auto &[ms_type, ms_object] : sprites.items() )
  {
    if ( not ms_object.contains( "spritesheet" ) ) throw std::runtime_error( "Missing 'spritesheet' from " + json_path.string() );
    const auto &spritesheet_json = ms_object.at( "spritesheet" );
    // SpriteSheet new_ms = spritesheet_json.get<SpriteSheet>();
    SpriteSheet new_ss{ ms_type,
                        Utils::JsonDeserializer::get_string( spritesheet_json, "displayname" ),
                        Utils::JsonDeserializer::get_float_array( spritesheet_json, "zorder" ),
                        Utils::JsonDeserializer::get_string( spritesheet_json, "texture_path" ),
                        Utils::JsonDeserializer::get_u32_array( spritesheet_json, "sprite_indices" ),
                        sf::Vector2i( Utils::JsonDeserializer::get_vector2i( spritesheet_json, "grid_size" ) ),
                        static_cast<unsigned int>( Utils::JsonDeserializer::get_int( spritesheet_json, "sprites_per_frame" ) ),
                        static_cast<unsigned int>( Utils::JsonDeserializer::get_int( spritesheet_json, "sprites_per_sequence" ) ),
                        Utils::JsonDeserializer::get_bool_array( spritesheet_json, "solid_mask" ),
                        Utils::JsonDeserializer::get_vector2i( spritesheet_json, "door_position", sf::Vector2i{} )

    };

    // new_ss.set_sprite_type( ms_type );
    SPDLOG_DEBUG( "Loaded sprite metadata for type: {}, tiles: {}", new_ss.get_sprite_type(), new_ss.get_sprite_count() );
    m_sprite_metadata_map[ms_type] = std::move( new_ss );
  }

  create_error_sprite();
}

void SpriteFactory::create_error_sprite()
{
  // Create procedural error texture (bright magenta/black checkerboard)
  sf::Image error_image( { 16, 16 }, sf::Color::Magenta );

  // Add black checkerboard pattern for visibility
  for ( unsigned int y = 0; y < 16; ++y )
  {
    for ( unsigned int x = 0; x < 16; ++x )
    {
      if ( ( x / 4 + y / 4 ) % 2 == 0 ) { error_image.setPixel( { x, y }, sf::Color::Black ); }
    }
  }

  // Create texture from image and save it
  [[maybe_unused]] bool result = m_error_texture.loadFromImage( error_image );

  // Create error sprite using the procedural texture
  m_error_metadata = SpriteSheet{ "ERROR_SPRITE",  "Error Sprite", { 0.0 },
                                  m_error_texture, // Use the in-memory texture
                                  { 0 },           { 1, 1 },       1,       1, {} };
}

std::pair<SpriteMetaType, std::size_t> SpriteFactory::get_random_type_and_texture_index( std::vector<SpriteMetaType> type_list )
{
  const SpriteSheet &selected_data = get_random_spritedata( type_list );

  // Find the type that corresponds to this data
  for ( const auto &[type, metadata] : m_sprite_metadata_map )
  {
    if ( metadata.get_sprite_type() == selected_data.get_sprite_type() )
    {
      Cmp::RandomInt random_picker( 0, static_cast<int>( selected_data.get_sprite_count() ) - 1 );
      return { type, random_picker.gen() };
    }
  }
  // Fallback to error sprite if not found

  SPDLOG_ERROR( "Could not find matching sprite types {} in map, returning error sprite", fmt::join( type_list, ", " ) );
  return { "ERROR_SPRITE", 0 };
}

SpriteMetaType SpriteFactory::get_random_type( std::vector<SpriteMetaType> type_list )
{
  const SpriteSheet &selected_data = get_random_spritedata( std::move( type_list ) );

  return selected_data.get_sprite_type();
}

std::vector<SpriteMetaType> SpriteFactory::get_all_sprite_types_by_pattern( const std::string &pattern )
{
  std::vector<SpriteMetaType> types;

  try
  {
    // Try to use as regex first
    std::regex pattern_regex( pattern );

    for ( const auto &[type, _] : m_sprite_metadata_map )
    {
      if ( std::regex_search( type, pattern_regex ) ) { types.push_back( type ); }
    }
  } catch ( const std::regex_error &e )
  {
    // If regex fails, fallback to substring matching (current behavior)
    SPDLOG_DEBUG( "Pattern '{}' is not valid regex, using substring matching", pattern );
    for ( const auto &[type, _] : m_sprite_metadata_map )
    {
      if ( type.find( pattern ) != std::string::npos ) { types.push_back( type ); }
    }
  }

  return types;
}

const Sprites::SpriteSheet &SpriteFactory::get_spritesheet_by_type( const SpriteMetaType &type, std::source_location loc )
{
  return get_spritedata_by_type( type, loc );
}

std::vector<SpriteMetaType> SpriteFactory::get_all_sprite_types()
{
  std::vector<SpriteMetaType> types;
  types.reserve( m_sprite_metadata_map.size() );

  for ( const auto &[type, _] : m_sprite_metadata_map )
  {
    types.push_back( type );
  }
  return types;
}

std::unordered_set<SpriteMetaType> SpriteFactory::get_all_sprite_types_set()
{
  std::unordered_set<SpriteMetaType> types;

  for ( const auto &pair : m_sprite_metadata_map )
  {
    types.insert( pair.first );
  }
  return types;
}

const SpriteSheet &SpriteFactory::get_spritedata_by_type( const SpriteMetaType &type, std::source_location loc )
{
  auto it = m_sprite_metadata_map.find( type );
  if ( it != m_sprite_metadata_map.end() ) { return it->second; }

  SPDLOG_ERROR( "Failed to locate sprite '{}'\n. Called from: {}:{} in '{}'", type, loc.file_name(), loc.line(), loc.function_name() );
  std::ostringstream ss;
  ss << "Failed to locate sprite '" << type << "'. Called from " << loc.file_name() << ":" << loc.line() << " in '" << loc.function_name() << "'";
  throw std::runtime_error( ss.str() );
}

const SpriteSheet &SpriteFactory::get_random_spritedata( std::vector<SpriteMetaType> type_list )
{
  if ( type_list.empty() )
  {
    SPDLOG_ERROR( "Cannot get random sprite from empty type list, returning error sprite" );
    return m_error_metadata;
  }

  try
  {
    Cmp::RandomInt picker( 0.0f, static_cast<int>( type_list.size() ) - 1 );
    int pick = picker.gen();
    return get_spritedata_by_type( type_list[pick] );
  } catch ( ... )
  {
    // fallback
    return get_spritedata_by_type( type_list.back() );
  }
}

} // namespace Game::Sprites
