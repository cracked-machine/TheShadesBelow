#include <Systems/Render/UiData.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <algorithm>
#include <cmath>
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

  // the map's own pixel size is the design space its objects were laid out in
  if ( ui_tilemap_json.contains( "width" ) and ui_tilemap_json.contains( "tilewidth" ) and ui_tilemap_json.contains( "height" ) and
       ui_tilemap_json.contains( "tileheight" ) )
  {
    const sf::Vector2f map_size( static_cast<float>( get_int( ui_tilemap_json, "width" ) * get_int( ui_tilemap_json, "tilewidth" ) ),
                                 static_cast<float>( get_int( ui_tilemap_json, "height" ) * get_int( ui_tilemap_json, "tileheight" ) ) );
    if ( map_size.x > 0.f and map_size.y > 0.f ) { m_design_size = map_size; }
  }

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
                              get_int_property( object, "sprite_index" ), static_cast<float>( get_int_property( object, "sprite_scale" ) ) );
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

  m_design_outlines = m_outlines;
  m_design_labels = m_labels;
  m_design_texts = m_texts;
  m_design_meters = m_meters;
  m_design_icons = m_icons;
  m_design_tables = m_tables;
  m_design_tabs = m_tabs;
}

float UiData::ui_scale( sf::Vector2u display_res, sf::Vector2f design )
{
  if ( display_res.x == 0u or display_res.y == 0u or design.x <= 0.f or design.y <= 0.f ) return 1.f;
  return std::min( static_cast<float>( display_res.x ) / design.x, static_cast<float>( display_res.y ) / design.y );
}

void UiData::sync_to( sf::Vector2u display_res, sf::Vector2f anchor_offset )
{
  if ( display_res.x == 0u or display_res.y == 0u ) return;
  if ( display_res == m_scaled_for and anchor_offset == m_anchor_offset ) return;
  // an anchor that follows the mouse changes every frame, so only log resolution changes
  const bool resolution_changed = display_res != m_scaled_for;
  m_scaled_for = display_res;
  m_anchor_offset = anchor_offset;

  // uniform scale so sprites and text are not distorted on a different aspect ratio (e.g. 16:10), centred in the spare space
  m_scale = ui_scale( display_res, m_design_size );
  m_offset = ( sf::Vector2f( display_res ) - ( m_design_size * m_scale ) ) / 2.f;
  if ( resolution_changed ) SPDLOG_DEBUG( "UiData scaled to {}x{}: scale {}, offset {},{}", display_res.x, display_res.y, m_scale, m_offset.x, m_offset.y );

  const float scale = m_scale;
  const sf::Vector2f offset = m_offset + m_anchor_offset;
  const auto scale_rect = [scale, offset]( const sf::FloatRect &rect ) { return sf::FloatRect( ( rect.position * scale ) + offset, rect.size * scale ); };
  // font sizes are whole pixels and must stay visible
  const auto scale_font = [scale]( int font_size ) { return std::max( 1, static_cast<int>( std::lround( static_cast<float>( font_size ) * scale ) ) ); };
  // keep a non-zero (possibly negative, i.e. inward) outline at least 1px thick
  const auto scale_thickness = [scale]( int thickness )
  {
    if ( thickness == 0 ) return 0;
    const int scaled = std::max( 1, static_cast<int>( std::lround( static_cast<float>( std::abs( thickness ) ) * scale ) ) );
    return thickness < 0 ? -scaled : scaled;
  };

  m_outlines = m_design_outlines;
  for ( auto &outline : m_outlines )
  {
    outline.rect = scale_rect( outline.rect );
    outline.line_thickness = scale_thickness( outline.line_thickness );
  }
  m_labels = m_design_labels;
  for ( auto &label : m_labels )
  {
    label.rect = scale_rect( label.rect );
    label.font_size = scale_font( label.font_size );
  }
  m_texts = m_design_texts;
  for ( auto &text : m_texts )
  {
    text.rect = scale_rect( text.rect );
    text.font_size = scale_font( text.font_size );
  }
  m_meters = m_design_meters;
  for ( auto &meter : m_meters ) { meter.rect = scale_rect( meter.rect ); }
  m_icons = m_design_icons;
  for ( auto &icon : m_icons )
  {
    icon.rect = scale_rect( icon.rect );
    icon.scale *= scale;
  }
  m_tables = m_design_tables;
  for ( auto &table : m_tables )
  {
    table.rect = scale_rect( table.rect );
    table.font_size = scale_font( table.font_size );
  }
  m_tabs = m_design_tabs;
  for ( auto &tab : m_tabs ) { tab.rect = scale_rect( tab.rect ); }
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