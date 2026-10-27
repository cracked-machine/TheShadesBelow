#include <Components/Random.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/JsonDeserializer.hpp>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <utility>

namespace Game::Sys
{

SpriteStore::SpriteStore( std::filesystem::path json_file_path )
    : StoreSingleton<SpriteStore, Sprites::SpriteSheet>( std::move( json_file_path ) )
{
  init_store();
  SPDLOG_DEBUG( "SpriteStore initialized" );
}

void SpriteStore::init_store()
{
  const auto &json_path = m_json_file_path;
  auto j = Utils::JsonDeserializer::load_json_file( json_path );

  if ( not j.contains( "sprites" ) ) throw std::runtime_error( "Missing 'sprites' from " + json_path.string() );
  const auto &sprites = j.at( "sprites" );
  for ( const auto &[ms_type, ms_object] : sprites.items() )
  {
    if ( not ms_object.contains( "spritesheet" ) ) throw std::runtime_error( "Missing 'spritesheet' from " + json_path.string() );
    const auto &spritesheet_json = ms_object.at( "spritesheet" );

    // "AbsoluteZOrder" and "RelativeZOrder" are mutually exclusive; neither means sort by y-position
    const bool has_absolute = spritesheet_json.contains( "AbsoluteZOrder" );
    const bool has_relative = spritesheet_json.contains( "RelativeZOrder" );
    if ( has_absolute and has_relative )
    {
      throw std::runtime_error( "Sprite '" + ms_type + "' sets both 'AbsoluteZOrder' and 'RelativeZOrder' in " + json_path.string() );
    }
    auto zorder_mode = Sprites::ZOrderMode::None;
    if ( has_absolute ) zorder_mode = Sprites::ZOrderMode::Absolute;
    else if ( has_relative ) zorder_mode = Sprites::ZOrderMode::Relative;
    const char *zorder_field = has_absolute ? "AbsoluteZOrder" : "RelativeZOrder";

    // Sprites::SpriteSheet new_ms = spritesheet_json.get<SpriteSheet>();
    Sprites::SpriteSheet new_ss{ SpriteKey( ms_type ),
                                 Utils::JsonDeserializer::get_string( spritesheet_json, "displayname" ),
                                 zorder_mode,
                                 Utils::JsonDeserializer::get_float_array( spritesheet_json, zorder_field ),
                                 Utils::JsonDeserializer::get_string( spritesheet_json, "texture_path" ),
                                 Utils::JsonDeserializer::get_u32_array( spritesheet_json, "sprite_indices" ),
                                 sf::Vector2i( Utils::JsonDeserializer::get_vector2i( spritesheet_json, "grid_size" ) ),
                                 static_cast<unsigned int>( Utils::JsonDeserializer::get_int( spritesheet_json, "indices_per_sequence" ) ),
                                 Utils::JsonDeserializer::get_bool_array( spritesheet_json, "walking_collision_mask" ),
                                 Utils::JsonDeserializer::get_bool_array( spritesheet_json, "flying_collision_mask" ),
                                 Utils::JsonDeserializer::get_vector2i( spritesheet_json, "door_position", sf::Vector2i{} ),
                                 Utils::JsonDeserializer::get_vector2f_array( spritesheet_json, "particle_offsets" ),
                                 Utils::JsonDeserializer::get_float( spritesheet_json, "framerate" )

    };

    // new_ss.set_sprite_type( ms_type );
    SPDLOG_DEBUG( "Loaded sprite metadata for type: {}, tiles: {}", new_ss.get_sprite_type(), new_ss.get_sprite_count() );
    m_store[SpriteKey( ms_type )] = std::move( new_ss );
  }
}

} // namespace Game::Sys
