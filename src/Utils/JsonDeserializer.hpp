#ifndef SRC_UTILS_JSONDESERIALIZER_HPP__
#define SRC_UTILS_JSONDESERIALIZER_HPP__

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <cstdint>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <source_location>
#include <string>

namespace Game::Utils
{

//! @brief Static helper class for loading JSON files and extracting typed, validated fields from
//! nlohmann::json objects. Every accessor throws std::runtime_error (tagged with the caller's
//! std::source_location) if a required field is missing or of the wrong type.
class JsonDeserializer
{
public:
  //! @brief Load and parse a JSON file from disk.
  //! @param path Path to the JSON file.
  //! @return nlohmann::json The parsed JSON document.
  //! @throws std::runtime_error if the file does not exist or cannot be opened.
  static nlohmann::json load_json_file( const std::filesystem::path &path );

  //! @brief Get a required string field.
  //! @param j The JSON object to read from.
  //! @param field The field name.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return std::string The field's value.
  //! @throws std::runtime_error if the field is missing or not a string.
  static std::string get_string( const nlohmann::json &j, const std::string &field, std::source_location loc = std::source_location::current() );

  //! @brief Get a required field that is either a single string or an array of strings.
  //! @param j The JSON object to read from.
  //! @param field The field name.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return std::vector<std::string> The values; a single string yields a one-element list.
  //! @throws std::runtime_error if the field is missing, or is neither a string nor an array of strings.
  static std::vector<std::string> get_string_or_list( const nlohmann::json &j, const std::string &field,
                                                      std::source_location loc = std::source_location::current() );

  //! @brief Get a required numeric field as a float.
  //! @param j The JSON object to read from.
  //! @param field The field name.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return float The field's value.
  //! @throws std::runtime_error if the field is missing or not a number.
  static float get_float( const nlohmann::json &j, const std::string &field, std::source_location loc = std::source_location::current() );

  //! @brief Get a required numeric field as an int.
  //! @param j The JSON object to read from.
  //! @param field The field name.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return int The field's value.
  //! @throws std::runtime_error if the field is missing or not a number.
  static int get_int( const nlohmann::json &j, const std::string &field, std::source_location loc = std::source_location::current() );

  //! @brief Get a required boolean field.
  //! @param j The JSON object to read from.
  //! @param field The field name.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return bool The field's value.
  //! @throws std::runtime_error if the field is missing or not a boolean.
  static bool get_bool( const nlohmann::json &j, const std::string &field, std::source_location loc = std::source_location::current() );

  //! @brief Get a required array field.
  //! @param j The JSON object to read from.
  //! @param field The field name.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return nlohmann::json A copy of the array.
  //! @throws std::runtime_error if the field is missing or not an array.
  static nlohmann::json get_array( const nlohmann::json &j, const std::string &field,
                                    std::source_location loc = std::source_location::current() );

  //! @brief Get a required color field, given as a `#RRGGBB` or `#RRGGBBAA` (or without the leading `#`) hex string.
  //! @param j The JSON object to read from.
  //! @param field The field name.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return sf::Color The parsed color.
  //! @throws std::runtime_error if the field is missing, not a string, or not a valid hex color.
  static sf::Color get_color( const nlohmann::json &j, const std::string &field, std::source_location loc = std::source_location::current() );

  //! @brief Build a rectangle from an object's "x", "y", "width" and "height" fields.
  //! @param object The JSON object holding "x", "y", "width" and "height" fields.
  //! @return sf::FloatRect The rectangle described by the object.
  //! @throws std::runtime_error if any of the required fields is missing or not a number.
  static sf::FloatRect get_float_rect( const nlohmann::json &object );

  //! @brief Get a required array field as a list of ints, reading each element's "value" field.
  //! @param j The JSON object to read from.
  //! @param field The field name (an array of objects, each with a "value" field).
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return std::vector<int> The list of values.
  static std::vector<int> get_int_list( const nlohmann::json &j, std::string field, std::source_location loc = std::source_location::current() );

  //! @brief Get an optional array field as a list of bools.
  //! @param j The JSON object to read from.
  //! @param field The field name.
  //! @param loc Unused (kept for signature consistency with the other array getters).
  //! @return std::vector<bool> The array's values, or an empty vector if the field is absent.
  static std::vector<bool> get_bool_array( const nlohmann::json &j, std::string field, std::source_location loc = std::source_location::current() );

  //! @brief Get a required array field as a list of uint32_t.
  //! @param j The JSON object to read from.
  //! @param field The field name.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return std::vector<uint32_t> The array's values.
  //! @throws std::runtime_error if the field is missing.
  static std::vector<uint32_t> get_u32_array( const nlohmann::json &j, std::string field,
                                              std::source_location loc = std::source_location::current() );

  //! @brief Get an optional array field as a list of floats.
  //! @param j The JSON object to read from.
  //! @param field The field name.
  //! @param loc Unused (kept for signature consistency with the other array getters).
  //! @return std::vector<float> The array's values, or an empty vector if the field is absent.
  static std::vector<float> get_float_array( const nlohmann::json &j, std::string field, std::source_location loc = std::source_location::current() );

  //! @brief Get an optional array field as a list of 2D float vectors, each read from its "x" and "y" sub-fields.
  //! @param j The JSON object to read from.
  //! @param field The field name.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return std::vector<sf::Vector2f> The array's values, or an empty vector if the field is absent.
  //! @throws std::runtime_error if an element is missing its "x"/"y" sub-fields.
  static std::vector<sf::Vector2f> get_vector2f_array( const nlohmann::json &j, std::string field,
                                                       std::source_location loc = std::source_location::current() );

  //! @brief Get a required string value from a Tiled-style "properties" array, by property name.
  //! @param j The JSON object holding a "properties" array.
  //! @param field The property's "name" to look up.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return std::string The matching property's "value".
  //! @throws std::runtime_error if "properties" or the named property is missing.
  static std::string get_string_property( const nlohmann::json &j, const std::string &field,
                                          std::source_location loc = std::source_location::current() );

  //! @brief Get a required int value from a Tiled-style "properties" array, by property name.
  //! @param j The JSON object holding a "properties" array.
  //! @param field The property's "name" to look up.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return int The matching property's "value".
  //! @throws std::runtime_error if "properties" or the named property is missing.
  static int get_int_property( const nlohmann::json &j, const std::string &field, std::source_location loc = std::source_location::current() );

  //! @brief Get a required color value from a Tiled-style "properties" array, by property name.
  //! @param j The JSON object holding a "properties" array.
  //! @param field The property's "name" to look up.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return sf::Color The matching property's "value", parsed as a hex color.
  //! @throws std::runtime_error if "properties" or the named property is missing.
  static sf::Color get_color_property( const nlohmann::json &j, const std::string &field,
                                       std::source_location loc = std::source_location::current() );

  //! @brief Get a required int-list value from a Tiled-style "properties" array, by property name.
  //! @param j The JSON object holding a "properties" array.
  //! @param field The property's "name" to look up.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return std::vector<int> The matching property's "value", as a list of ints.
  //! @throws std::runtime_error if "properties" or the named property is missing.
  static std::vector<int> get_int_list_property( const nlohmann::json &j, const std::string &field,
                                                 std::source_location loc = std::source_location::current() );

  //! @brief Resolve a relative path against the game resource directory, stripping any "." or ".." components.
  //! @param rel The relative path to resolve.
  //! @return std::filesystem::path The path rooted at Game::Constants::res_dir.
  static std::filesystem::path get_abs_path( const std::filesystem::path &rel );

  //! @brief Get a required field as a 2D integer vector, read from its "x" and "y" sub-fields.
  //! @param j The JSON object to read from.
  //! @param field The field name.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return sf::Vector2i The parsed vector.
  //! @throws std::runtime_error if the field or its "x"/"y" sub-fields are missing.
  static sf::Vector2i get_vector2i( const nlohmann::json &j, const std::string &field, std::source_location loc = std::source_location::current() );

  //! @brief Get an optional field as a 2D integer vector, read from its "x" and "y" sub-fields.
  //! @param j The JSON object to read from.
  //! @param field The field name.
  //! @param default_val Value returned if `field` is absent.
  //! @param loc Caller location, used in error messages (defaults to the call site).
  //! @return sf::Vector2i The parsed vector, or `default_val` if the field is absent.
  static sf::Vector2i get_vector2i( const nlohmann::json &j, const std::string &field, sf::Vector2i default_val,
                                    std::source_location loc = std::source_location::current() );

protected:
  //! @brief Non-public destructor — this class is a static-only helper, not meant to be instantiated or owned polymorphically.
  ~JsonDeserializer() = default;

private:
  //! @brief Format a source location as a "file:line " prefix for error messages.
  //! @param loc The source location to format.
  //! @return std::string The formatted prefix.
  static std::string make_loc_string( const std::source_location &loc );

  //! @brief Throw std::runtime_error if `j` does not contain `field`.
  //! @param j The JSON object to check.
  //! @param field The field name that must be present.
  //! @param loc Caller location, used in the error message.
  static void check_contains( const nlohmann::json &j, const std::string &field, const std::source_location &loc );
};

} // namespace Game::Utils

#endif // SRC_UTILS_JSONDESERIALIZER_HPP__
