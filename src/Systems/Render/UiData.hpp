#ifndef SRC_SYSTEMS_RENDER_UIDATA_HPP__
#define SRC_SYSTEMS_RENDER_UIDATA_HPP__

#include <SFML/System/Angle.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/JsonDeserializer.hpp>
#include <nlohmann/json_fwd.hpp>

#include <string>
#include <utility>
#include <vector>

namespace Game::Render
{

//! @brief Loads and holds UI layout data (outlines, labels, texts, meters and icons) deserialized from a Tiled JSON UI map file.
class UiData : public Utils::JsonDeserializer
{
public:
  //! @brief A rectangular outline element in the UI layout (e.g. a panel border).
  struct Outline
  {
    //! @brief Position and size of the outline rectangle.
    sf::FloatRect rect;
    //! @brief Identifying name of this outline, as given in the UI layout file.
    std::string name;
    //! @brief Fill colour of the rectangle.
    sf::Color fill_color;
    //! @brief Outline (border) colour of the rectangle.
    sf::Color line_color;
    //! @brief Outline (border) thickness, in pixels.
    int line_thickness;
    //! @brief Clockwise rotation about the rectangle's top-left corner, matching Tiled's object rotation.
    sf::Angle rotation;
  };

  //! @brief A sprite icon element in the UI layout.
  struct Icon
  {
    //! @brief Position and size of the icon.
    sf::FloatRect rect;
    //! @brief Identifying name of this icon, as given in the UI layout file.
    std::string name;
    //! @brief Sprite meta-type used to look up the icon's texture.
    Sys::SpriteKey type;
    //! @brief Sprite index within the sprite sheet.
    int index;
    //! @brief Scale factor applied to the icon.
    int scale;
  };

  //! @brief A dynamic text label element in the UI layout (e.g. a stat value driven by game state).
  struct Label
  {
    //! @brief Position and size of the label.
    sf::FloatRect rect;
    //! @brief Identifying name of this label, as given in the UI layout file.
    std::string name;
    //! @brief Font size, in pixels, for the label text.
    int font_size;
    //! @brief Text alignment for the label, as given in the UI layout file (e.g. "left", "center").
    std::string align;
  };

  //! @brief A bar/meter element in the UI layout (e.g. health, fear).
  struct Meter
  {
    //! @brief Position and size of the meter.
    sf::FloatRect rect;
    //! @brief Identifying name of this meter, as given in the UI layout file.
    std::string name;
  };

  //! @brief A static text element in the UI layout.
  struct Text
  {
    //! @brief Position and size of the text.
    sf::FloatRect rect;
    //! @brief Identifying name of this text element, as given in the UI layout file.
    std::string name;
    //! @brief The static string content to display.
    std::string value;
    //! @brief Font size, in pixels, for the text.
    int font_size;
  };

  //! @brief A table element in the UI layout: one row per entry of `rows`, drawn as an ImGui table.
  struct Table
  {
    //! @brief Position and size of the table.
    sf::FloatRect rect;
    //! @brief Identifying name of this table, as given in the UI layout file.
    std::string name;
    //! @brief The table's rows, in order, as (key, caption) pairs, e.g. ("destroy_action", "Chop"). Parsed from the
    //! "rows" property, a comma separated list of "key=Caption" entries.
    std::vector<std::pair<std::string, std::string>> rows;
    //! @brief Font size, in pixels, for the table text.
    int font_size;
  };

  //! @brief A tab bar element in the UI layout, drawn as an ImGui tab bar.
  struct Tab
  {
    //! @brief Position and size of the tab bar.
    sf::FloatRect rect;
    //! @brief Identifying name of this tab bar, as given in the UI layout file.
    std::string name;
  };

  //! @brief Construct a new UiData object and deserialize the given UI map file.
  //! @param map_file
  UiData( const std::filesystem::path &map_file );

  //! @brief Parse the Tiled JSON UI map file at `scene_tiledata_path` and populate the outline, label, text, meter and icon lists.
  //! @param scene_tiledata_path
  void deserialize( const std::filesystem::path &scene_tiledata_path );

  //! @brief Outline (panel border) elements parsed from the UI layout file.
  std::vector<Outline> m_outlines;

  //! @brief Dynamic text label elements parsed from the UI layout file.
  std::vector<Label> m_labels;

  //! @brief Static text elements parsed from the UI layout file.
  std::vector<Text> m_texts;

  //! @brief Bar/meter elements parsed from the UI layout file.
  std::vector<Meter> m_meters;

  //! @brief Sprite icon elements parsed from the UI layout file.
  std::vector<Icon> m_icons;

  //! @brief Table elements parsed from the UI layout file.
  std::vector<Table> m_tables;

  //! @brief Tab bar elements parsed from the UI layout file.
  std::vector<Tab> m_tabs;

private:
  //! @brief Parse a ui_table "rows" property, e.g. "burn_action=Burn, destroy_action=Chop".
  //! @param rows The property value.
  //! @return The (key, caption) pairs, in order. An entry without '=' uses the key as its caption.
  static std::vector<std::pair<std::string, std::string>> parse_table_rows( const std::string &rows );
};

} // namespace Game::Render

#endif // SRC_SYSTEMS_RENDER_UIDATA_HPP__
