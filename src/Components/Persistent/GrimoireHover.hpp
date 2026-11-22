#ifndef SRC_COMPONENTS_PERSISTENT_GRIMOIREHOVER_HPP__
#define SRC_COMPONENTS_PERSISTENT_GRIMOIREHOVER_HPP__

#include <Utils/ImGuiStepSlider.hpp>

#include <array>
#include <cstdint>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace Game::Cmp::Persist
{

//! @brief How the grimoire hover overlay (the grimoire subpage of the plant or NPC under the mouse) is shown: at its fixed
//! position in res/ui/grimoire_plant_hover.json or res/ui/grimoire_npc_hover.json ("Anchored"), beside the mouse cursor
//! ("FollowMouse"), or not at all ("Disabled").
//! @note Read every frame by Sys::GrimoireOverlaySystem::update.
class GrimoireHover
{
public:
  //! @brief The hover overlay placement options, in the same order as kModeNames.
  enum class Mode : uint8_t { ANCHORED, FOLLOW_MOUSE, DISABLED };

  //! @brief The persist.json value and combobox label of each Mode, indexed by the Mode's underlying value.
  static constexpr std::array<std::string_view, 3> kModeNames{ "Anchored", "FollowMouse", "Disabled" };

  //! @brief Construct the grimoire hover setting (its value is set from persist.json).
  GrimoireHover() = default;

  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const { return "GrimoireHover"; }

  //! @brief Get the current hover mode.
  //! @return Mode the current hover mode
  [[nodiscard]] Mode get_value() const { return m_mode; }

  //! @brief Deserialize the hover mode from json (res/json/persist.json).
  //! An unrecognised "value" is logged and leaves the mode unchanged.
  //! @param json_data the json object to read the "value" and "detail" fields from
  void deserialize( const nlohmann::json &json_data )
  {
    if ( json_data.contains( "value" ) and json_data["value"].is_string() )
    {
      const auto value = json_data["value"].get<std::string>();
      bool found = false;
      for ( std::size_t i = 0; i < kModeNames.size(); ++i )
      {
        if ( kModeNames.at( i ) == value )
        {
          m_mode = static_cast<Mode>( i );
          found = true;
        }
      }
      if ( not found ) { SPDLOG_WARN( "{}: unknown value '{}'", class_name(), value ); }
    }
    else { SPDLOG_WARN( "{}: missing or non-string 'value'", class_name() ); }
    if ( json_data.contains( "detail" ) and json_data["detail"].is_string() ) { m_detail = json_data["detail"].get<std::string>(); }
  }

  //! @brief Serialize the hover mode to json (res/json/persist.json).
  //! @return nlohmann::json object with "type", "value" and "detail" fields
  nlohmann::json serialize() const
  {
    nlohmann::json json_data;
    json_data["type"] = "string";
    json_data["value"] = std::string( kModeNames.at( static_cast<std::size_t>( m_mode ) ) );
    json_data["detail"] = m_detail;
    return json_data;
  }

  //! @brief Draw a combobox selecting the hover mode, with the detail as its label.
  void render_widget()
  {
    const char *label = m_detail.empty() ? "<Invalid Label>" : m_detail.c_str();
    Utils::imgui_labelled_row( this, label, [&]( float width )
    {
      const auto idx = static_cast<std::size_t>( m_mode );
      ImGui::SetNextItemWidth( width );
      if ( ImGui::BeginCombo( "##value", kModeNames.at( idx ).data() ) )
      {
        for ( std::size_t i = 0; i < kModeNames.size(); ++i )
        {
          const bool selected = i == idx;
          if ( ImGui::Selectable( kModeNames.at( i ).data(), selected ) ) { m_mode = static_cast<Mode>( i ); }
          if ( selected ) { ImGui::SetItemDefaultFocus(); }
        }
        ImGui::EndCombo();
      }
    } );
  }

private:
  //! @brief the current hover mode
  Mode m_mode{ Mode::ANCHORED };
  //! @brief human-readable detail/description string shown in the UI
  std::string m_detail;
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_GRIMOIREHOVER_HPP__
