#ifndef SRC_COMPONENTS_PERSISTENT_DISPLAYRESOLUTION_HPP__
#define SRC_COMPONENTS_PERSISTENT_DISPLAYRESOLUTION_HPP__

#include <SFML/System/Vector2.hpp>
#include <nlohmann/json.hpp>
#include <string>
namespace Game::Cmp::Persist
{

//! @brief Persistent window/screen resolution setting, stored as an sf::Vector2u (width, height).
class DisplayResolution : public sf::Vector2u
{
public:
  //! @brief Construct with a zero-initialized resolution.
  DisplayResolution() = default;
  //! @brief Construct from explicit width/height values.
  //! @param width resolution width in pixels
  //! @param height resolution height in pixels
  DisplayResolution( unsigned int width, unsigned int height )
      : sf::Vector2u( width, height ) {};
  //! @brief Construct from an existing sf::Vector2u.
  //! @param vec the resolution as an sf::Vector2u
  DisplayResolution( sf::Vector2u vec )
      : sf::Vector2u( vec ) {};

  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "DisplayResolution"
  std::string class_name() const { return "DisplayResolution"; }

  //! @brief Get a human-readable "x: <x> y: <y>" representation of the resolution.
  //! @return std::string& the formatted resolution string
  std::string &get_value()
  {
    std::stringstream ss;
    ss << "x: " << x << " y: " << y;
    static std::string str = ss.str();
    return str;
  };

  //! @brief Deserialize the resolution from json (res/json/persist.json), falling back
  //! to a low default resolution if the stored value is unreasonably small.
  //! @param json_data the json object to read the "value" field from
  void deserialize( const nlohmann::json &json_data )
  {
    if ( json_data.contains( "value" ) && json_data["value"].is_object() )
    {
      unsigned int new_x = json_data["value"].value( "x", 0u );
      unsigned int new_y = json_data["value"].value( "y", 0u );

      // we cannot use 0,0 as a resolution, so clamp to a fallback default,
      // use a silly low resolution so its obvious that something is wrong
      if ( new_x < 800 && new_y < 600 )
      {
        x = 800;
        y = 600;
      }
      else
      {
        x = new_x;
        y = new_y;
      }
    }
  }

  //! @brief Serialize the resolution to json (res/json/persist.json).
  //! @return nlohmann::json object with "type" set to "sf::Vector2u" and "value" holding x/y
  virtual nlohmann::json serialize() const
  {
    nlohmann::json json_data;
    json_data["type"] = "sf::Vector2u";
    json_data["value"]["x"] = x;
    json_data["value"]["y"] = y;
    return json_data;
  }

  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_DISPLAYRESOLUTION_HPP__
