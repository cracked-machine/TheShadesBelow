#ifndef SRC_COMPONENTS_PERSISTENT_ARMEDBLOCKCOLOURFILL_HPP__
#define SRC_COMPONENTS_PERSISTENT_ARMEDBLOCKCOLOURFILL_HPP__

#include <imgui.h>
#include <nlohmann/json.hpp>

namespace Game::Cmp::Persist
{

//! @brief Fill colour used to render armed bomb obstacle blocks.
class ArmedBlockColourFill : public sf::Color
{
public:
  //! @brief Construct with the default sf::Color (transparent black).
  ArmedBlockColourFill() = default;
  //! @brief Construct from an existing colour.
  //! @param rgb the fill colour
  ArmedBlockColourFill( sf::Color rgb )
      : sf::Color( rgb )
  {
  }

  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "ArmedBlockColourFill"
  [[nodiscard]] std::string class_name() const { return "ArmedBlockColourFill"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] std::string get_detail() const { return ""; }

  //! @brief Render an ImGui colour-edit widget bound to this colour's RGBA components.
  void render_widget()
  {
    std::array<float, 4> colour = { static_cast<float>( r ) / 255.f, static_cast<float>( g ) / 255.f, static_cast<float>( b ) / 255.f,
                                    static_cast<float>( a ) / 255.f };
    if ( ImGui::ColorEdit4( "ArmedBlockColourFill", colour.data() ) )
    {
      r = static_cast<uint8_t>( colour[0] * 255.f );
      g = static_cast<uint8_t>( colour[1] * 255.f );
      b = static_cast<uint8_t>( colour[2] * 255.f );
      a = static_cast<uint8_t>( colour[3] * 255.f );
    }
  }

  //! @brief Deserialise this colour from json (res/json/persist.json).
  //! @param json_data json object expected to contain a "value" object with r/g/b/a keys
  void deserialize( const nlohmann::json &json_data )
  {
    if ( json_data.contains( "value" ) and json_data["value"].is_object() )
    {
      r = json_data["value"].value( "r", 0u );
      g = json_data["value"].value( "g", 0u );
      b = json_data["value"].value( "b", 0u );
      a = json_data["value"].value( "a", 0u );
    }
  }

  //! @brief Serialise this colour to json (res/json/persist.json).
  //! @return nlohmann::json json object containing the type and r/g/b/a value
  [[nodiscard]] virtual nlohmann::json serialize() const
  {
    nlohmann::json json_data;
    json_data["type"] = "sf::Color";
    json_data["value"]["r"] = r;
    json_data["value"]["g"] = g;
    json_data["value"]["b"] = b;
    json_data["value"]["a"] = a;
    return json_data;
  }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_ARMEDBLOCKCOLOURFILL_HPP__
