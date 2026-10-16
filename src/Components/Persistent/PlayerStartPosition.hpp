#ifndef SRC_COMPONENTS_PERSISTENT_PLAYERSTARTPOSITION_HPP__
#define SRC_COMPONENTS_PERSISTENT_PLAYERSTARTPOSITION_HPP__

#include <SFML/System/Vector2.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>

namespace Game::Cmp::Persist
{

//! @brief Persistent setting for the player's spawn/start position in world space.
class PlayerStartPosition : public sf::Vector2f
{
public:
  //! @brief Construct a new PlayerStartPosition object.
  //! @param pos Initial start position.
  PlayerStartPosition( sf::Vector2f pos = { 0.0f, 0.0f } );

  //! @brief Returns the class name used for identification and JSON serialization.
  //! @return std::string The class name.
  std::string class_name() const { return "PlayerStartPosition"; }

  //! @brief Returns a human-readable "x: ... y: ..." representation of the position.
  //! @return std::string& Reference to a static formatted string.
  std::string &get_value();

  //! @brief Deserialise this setting from JSON (res/json/persist.json).
  //! @param json_data The JSON object to read from.
  void deserialize( const nlohmann::json &json_data );

  //! @brief Serialise this setting to JSON (res/json/persist.json).
  //! @return nlohmann::json The serialised representation.
  nlohmann::json serialize() const;

  //! @brief Returns the display detail string for this setting (currently unused/empty).
  //! @return std::string The detail string.
  const std::string get_detail() const { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_PLAYERSTARTPOSITION_HPP__
