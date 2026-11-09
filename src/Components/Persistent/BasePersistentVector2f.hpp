#ifndef SRC_COMPONENTS_PERSISTENT_BASEPERSISTENTVECTOR2F_HPP__
#define SRC_COMPONENTS_PERSISTENT_BASEPERSISTENTVECTOR2F_HPP__

#include <SFML/System/Vector2.hpp>
#include <imgui.h>
#include <nlohmann/json.hpp>

#include <string>
#include <utility>

namespace Game::Cmp::Persist
{

//! @brief Base for a persistent, tunable sf::Vector2f setting (res/json/persist.json "type": "sf::Vector2f").
//! @details "value" is an {x, y} object; "min_value"/"max_value" are scalars bounding both axes.
//! Derived classes only supply a default constructor passing their class name, which is the persist.json key.
class BasePersistentVector2f : public sf::Vector2f
{
public:
  //! @brief Construct with a zero vector.
  //! @param name the class name used for persistence/registry lookup
  explicit BasePersistentVector2f( std::string name )
      : m_name( std::move( name ) )
  {
  }

  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name
  [[nodiscard]] std::string class_name() const { return m_name; }
  //! @brief Get the human-readable detail/description text.
  //! @return std::string the detail string
  [[nodiscard]] std::string get_detail() const { return m_detail; }

  //! @brief Render an ImGui two-component slider bound to x and y.
  void render_widget()
  {
    ImGui::PushID( this );
    const char *label = m_detail.empty() ? m_name.c_str() : m_detail.c_str();
    const char *fmt = m_format.empty() ? "%.3f" : m_format.c_str();
    float xy[2] = { x, y };
    if ( ImGui::SliderFloat2( label, xy, m_min_value, m_max_value, fmt ) )
    {
      x = xy[0];
      y = xy[1];
    }
    ImGui::PopID();
  }

  //! @brief Deserialise this vector from json (res/json/persist.json).
  //! @param json_data json object expected to contain a "value" object with x/y keys
  void deserialize( const nlohmann::json &json_data )
  {
    if ( json_data.contains( "value" ) and json_data["value"].is_object() )
    {
      x = json_data["value"].value( "x", 0.f );
      y = json_data["value"].value( "y", 0.f );
    }
    m_min_value = json_data.value( "min_value", 0.f );
    m_max_value = json_data.value( "max_value", 0.f );
    m_detail = json_data.value( "detail", std::string{} );
    m_format = json_data.value( "format", std::string{} );
  }

  //! @brief Serialise this vector to json (res/json/persist.json).
  //! @return nlohmann::json json object containing the type, x/y value, bounds, detail and format
  [[nodiscard]] nlohmann::json serialize() const
  {
    nlohmann::json json_data;
    json_data["type"] = "sf::Vector2f";
    json_data["value"]["x"] = x;
    json_data["value"]["y"] = y;
    json_data["min_value"] = m_min_value;
    json_data["max_value"] = m_max_value;
    json_data["detail"] = m_detail;
    json_data["format"] = m_format;
    return json_data;
  }

private:
  //! @brief The class name used for persistence/registry lookup.
  std::string m_name;
  //! @brief Lower bound for both axes.
  float m_min_value{ 0.f };
  //! @brief Upper bound for both axes.
  float m_max_value{ 0.f };
  //! @brief Human-readable label shown in the settings menu.
  std::string m_detail;
  //! @brief ImGui format string used to display each axis.
  std::string m_format;
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_BASEPERSISTENTVECTOR2F_HPP__
