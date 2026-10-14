#include <Utils/Constants.hpp>
#include <Utils/JsonDeserializer.hpp>

#include <charconv>
#include <cstdint>
#include <fstream>
#include <spdlog/spdlog.h>

namespace Game::Utils
{

std::string JsonDeserializer::make_loc_string( const std::source_location &loc )
{
  return std::string( loc.file_name() ) + ":" + std::to_string( loc.line() ) + " ";
}

void JsonDeserializer::check_contains( const nlohmann::json &j, const std::string &field, const std::source_location &loc )
{
  if ( not j.contains( field ) ) throw std::runtime_error( make_loc_string( loc ) + "Missing JSON field: " + field );
}

nlohmann::json JsonDeserializer::load_json_file( const std::filesystem::path &path )
{
  if ( not std::filesystem::exists( path ) )
  {
    SPDLOG_ERROR( "JSON file does not exist: {}", path.string() );
    throw std::runtime_error( "JSON file not found: " + path.string() );
  }
  std::ifstream fs( path );
  if ( not fs.is_open() )
  {
    SPDLOG_ERROR( "Unable to open JSON file: {}", path.string() );
    throw std::runtime_error( "Cannot open JSON file: " + path.string() );
  }
  nlohmann::json json;
  fs >> json;
  return json;
}

std::string JsonDeserializer::get_string( const nlohmann::json &j, const std::string &field, std::source_location loc )
{
  check_contains( j, field, loc );
  if ( not j.at( field ).is_string() )
    throw std::runtime_error( make_loc_string( loc ) + "JSON field '" + field + "' is not a string, got: " + j.at( field ).type_name() );
  return j.at( field ).get<std::string>();
}

float JsonDeserializer::get_float( const nlohmann::json &j, const std::string &field, std::source_location loc )
{
  check_contains( j, field, loc );
  if ( not j.at( field ).is_number() )
    throw std::runtime_error( make_loc_string( loc ) + "JSON field '" + field + "' is not a number, got: " + j.at( field ).type_name() );
  return j.at( field ).get<float>();
}

int JsonDeserializer::get_int( const nlohmann::json &j, const std::string &field, std::source_location loc )
{
  check_contains( j, field, loc );
  if ( not j.at( field ).is_number() )
    throw std::runtime_error( make_loc_string( loc ) + "JSON field '" + field + "' is not a number, got: " + j.at( field ).type_name() );
  return j.at( field ).get<int>();
}

bool JsonDeserializer::get_bool( const nlohmann::json &j, const std::string &field, std::source_location loc )
{
  check_contains( j, field, loc );
  if ( not j.at( field ).is_boolean() )
    throw std::runtime_error( make_loc_string( loc ) + "JSON field '" + field + "' is not a boolean, got: " + j.at( field ).type_name() );
  return j.at( field ).get<bool>();
}

nlohmann::json JsonDeserializer::get_array( const nlohmann::json &j, const std::string &field, std::source_location loc )
{
  check_contains( j, field, loc );
  if ( not j.at( field ).is_array() )
    throw std::runtime_error( make_loc_string( loc ) + "JSON field '" + field + "' is not an array, got: " + j.at( field ).type_name() );
  return j.at( field );
}

std::vector<int> JsonDeserializer::get_int_list( const nlohmann::json &j, std::string field, std::source_location loc )
{
  std::vector<int> results;
  for ( const auto &value : j.at( field ) )
  {
    results.push_back( get_int( value, "value", loc ) );
  }
  return results;
}

std::vector<bool> JsonDeserializer::get_bool_array( const nlohmann::json &j, std::string field, [[maybe_unused]] std::source_location loc )
{
  // optional
  if ( not j.contains( field ) ) return {};
  return j.at( field ).get<std::vector<bool>>();
}

std::vector<uint32_t> JsonDeserializer::get_u32_array( const nlohmann::json &j, std::string field, std::source_location loc )
{
  check_contains( j, field, loc );
  return j.at( field ).get<std::vector<uint32_t>>();
}

std::vector<float> JsonDeserializer::get_float_array( const nlohmann::json &j, std::string field, std::source_location loc )
{
  check_contains( j, field, loc );
  return j.at( field ).get<std::vector<float>>();
}

sf::Color JsonDeserializer::get_color( const nlohmann::json &j, const std::string &field, std::source_location loc )
{
  auto hex_string = get_string( j, field, loc );
  if ( not hex_string.empty() && hex_string[0] == '#' ) hex_string = hex_string.substr( 1 );

  uint32_t value{};
  const auto [ptr, ec] = std::from_chars( hex_string.data(), hex_string.data() + hex_string.size(), value, 16 );
  if ( ec != std::errc{} ) throw std::runtime_error( "Invalid hex color string: " + hex_string );

  return { static_cast<uint8_t>( ( value >> 16 ) & 0xFF ), static_cast<uint8_t>( ( value >> 8 ) & 0xFF ),
           static_cast<uint8_t>( ( value >> 0 ) & 0xFF ), static_cast<uint8_t>( ( value >> 24 ) & 0xFF ) };
}

sf::FloatRect JsonDeserializer::get_float_rect( const nlohmann::json &object )
{
  return { { get_float( object, "x" ), get_float( object, "y" ) }, { get_float( object, "width" ), get_float( object, "height" ) } };
}

std::string JsonDeserializer::get_string_property( const nlohmann::json &j, const std::string &field, std::source_location loc )
{
  if ( not j.contains( "properties" ) )
    throw std::runtime_error( make_loc_string( loc ) + "Missing JSON property 'properties' in object " + get_string( j, "name" ) );
  for ( const auto &prop : j.at( "properties" ) )
    if ( get_string( prop, "name" ) == field ) return get_string( prop, "value" );
  throw std::runtime_error( make_loc_string( loc ) + "Missing JSON property field in object: " + field );
}

int JsonDeserializer::get_int_property( const nlohmann::json &j, const std::string &field, std::source_location loc )
{
  if ( not j.contains( "properties" ) )
    throw std::runtime_error( make_loc_string( loc ) + "Missing JSON property 'properties' in object " + get_string( j, "name" ) );
  for ( const auto &prop : j.at( "properties" ) )
    if ( get_string( prop, "name" ) == field ) return get_int( prop, "value" );
  throw std::runtime_error( make_loc_string( loc ) + "Missing JSON property field in object: " + field );
}

std::vector<int> JsonDeserializer::get_int_list_property( const nlohmann::json &j, const std::string &field, std::source_location loc )
{
  if ( not j.contains( "properties" ) )
    throw std::runtime_error( make_loc_string( loc ) + "Missing JSON property 'properties' in object " + get_string( j, "name" ) );
  for ( const auto &prop : j.at( "properties" ) )
    if ( get_string( prop, "name" ) == field ) return get_int_list( prop, "value" );
  throw std::runtime_error( make_loc_string( loc ) + "Missing JSON property field in object: " + field );
}

sf::Color JsonDeserializer::get_color_property( const nlohmann::json &j, const std::string &field, std::source_location loc )
{
  if ( not j.contains( "properties" ) )
    throw std::runtime_error( make_loc_string( loc ) + "Missing JSON property 'properties' in object " + get_string( j, "name" ) );
  for ( const auto &prop : j.at( "properties" ) )
    if ( get_string( prop, "name" ) == field ) return get_color( prop, "value" );
  throw std::runtime_error( make_loc_string( loc ) + "Missing JSON property field in object: " + field );
}

std::filesystem::path JsonDeserializer::get_abs_path( const std::filesystem::path &rel )
{
  // strip the relative dots and prepend with the resource dir
  std::filesystem::path result;
  for ( const auto &part : std::filesystem::path( rel ) )
  {
    if ( part == ".." || part == "." ) continue;
    result /= part;
  }
  return Game::Constants::res_dir / result;
}

sf::Vector2i JsonDeserializer::get_vector2i( const nlohmann::json &j, const std::string &field, std::source_location loc )
{
  check_contains( j, field, loc );
  const auto &obj = j.at( field );
  return { get_int( obj, "x", loc ), get_int( obj, "y", loc ) };
}

sf::Vector2i JsonDeserializer::get_vector2i( const nlohmann::json &j, const std::string &field, sf::Vector2i default_val, std::source_location loc )
{
  return j.contains( field ) ? JsonDeserializer::get_vector2i( j, field, loc ) : default_val;
}

} // namespace Game::Utils