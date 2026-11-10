#include <Systems/Render/UiData.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <filesystem>
#include <ranges>
#include <string_view>
#include <nlohmann/json.hpp>

namespace Game::Render
{

UiData::UiData( const std::filesystem::path &map_file ) { deserialize( map_file ); }

void UiData::deserialize( const std::filesystem::path &scene_tiledata_path )
{

  nlohmann::json ui_tilemap_json = load_json_file( scene_tiledata_path );
  if ( not ui_tilemap_json.contains( "layers" ) ) return;
  for ( const auto &layer : ui_tilemap_json.at( "layers" ) )
  {
    SPDLOG_DEBUG( "Found Layers" );
    if ( not layer.contains( "objects" ) ) continue;
    for ( const auto &object : layer.at( "objects" ) )
    {

      if ( not object.contains( "name" ) ) throw std::runtime_error( "Missing JSON property 'name' in object" );
      if ( not object.contains( "type" ) ) throw std::runtime_error( "Missing JSON property 'type' in object: " + get_string( object, "name" ) );
      if ( not object.contains( "x" ) ) throw std::runtime_error( "Missing JSON property 'x' in object: " + get_string( object, "name" ) );
      if ( not object.contains( "y" ) ) throw std::runtime_error( "Missing JSON property 'y' in object: " + get_string( object, "name" ) );
      if ( not object.contains( "width" ) ) throw std::runtime_error( "Missing JSON property 'width' in object: " + get_string( object, "name" ) );
      if ( not object.contains( "height" ) ) throw std::runtime_error( "Missing JSON property 'height' in object: " + get_string( object, "name" ) );

      if ( get_string( object, "type" ) == "ui_outline" )
      {
        SPDLOG_DEBUG( "Found ui_outline: {}", get_string( object, "name" ) );
        m_outlines.emplace_back( get_float_rect( object ), get_string( object, "name" ), get_color_property( object, "fill_color" ),
                                 get_color_property( object, "line_color" ), get_int_property( object, "line_thickness" ),
                                 sf::degrees( get_float( object, "rotation" ) ) );
      }
      if ( get_string( object, "type" ) == "ui_label" )
      {
        SPDLOG_DEBUG( "Found ui_label: {}", get_string( object, "name" ) );
        m_labels.emplace_back( get_float_rect( object ), get_string( object, "name" ), get_int_property( object, "font_size" ),
                               get_string_property( object, "align" ) );
      }
      if ( get_string( object, "type" ) == "ui_text" )
      {
        SPDLOG_DEBUG( "Found ui_text: {}", get_string( object, "name" ) );
        m_texts.emplace_back( get_float_rect( object ), get_string( object, "name" ), get_string_property( object, "value" ),
                              get_int_property( object, "font_size" ) );
      }
      if ( get_string( object, "type" ) == "ui_meter" )
      {
        SPDLOG_DEBUG( "Found ui_meter: {}", get_string( object, "name" ) );
        m_meters.emplace_back( get_float_rect( object ), get_string( object, "name" ) );
      }
      if ( get_string( object, "type" ) == "ui_icon" )
      {
        m_icons.emplace_back( get_float_rect( object ), get_string( object, "name" ), Sys::SpriteKey( get_string_property( object, "sprite_type" ) ),
                              get_int_property( object, "sprite_index" ), get_int_property( object, "sprite_scale" ) );
      }
      if ( get_string( object, "type" ) == "ui_table" )
      {
        SPDLOG_DEBUG( "Found ui_table: {}", get_string( object, "name" ) );
        m_tables.emplace_back( get_float_rect( object ), get_string( object, "name" ), parse_table_rows( get_string_property( object, "rows" ) ),
                               get_int_property( object, "font_size" ) );
      }
      if ( get_string( object, "type" ) == "ui_tab" )
      {
        SPDLOG_DEBUG( "Found ui_tab: {}", get_string( object, "name" ) );
        m_tabs.emplace_back( get_float_rect( object ), get_string( object, "name" ) );
      }
    }
  }
}

std::vector<std::pair<std::string, std::string>> UiData::parse_table_rows( const std::string &rows )
{
  const auto trim = []( std::string_view str )
  {
    const auto first = str.find_first_not_of( " \t" );
    if ( first == std::string_view::npos ) return std::string();
    const auto last = str.find_last_not_of( " \t" );
    return std::string( str.substr( first, last - first + 1 ) );
  };

  std::vector<std::pair<std::string, std::string>> parsed;
  for ( const auto entry : std::views::split( rows, ',' ) )
  {
    const std::string_view entry_view( entry.begin(), entry.end() );
    const auto equals = entry_view.find( '=' );
    std::string key = trim( entry_view.substr( 0, equals ) );
    if ( key.empty() ) continue;
    std::string caption = equals == std::string_view::npos ? key : trim( entry_view.substr( equals + 1 ) );
    parsed.emplace_back( std::move( key ), std::move( caption ) );
  }
  return parsed;
}

} // namespace Game::Render