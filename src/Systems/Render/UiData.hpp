#ifndef SRC_SYSTEMS_RENDER_UIDATA_HPP__
#define SRC_SYSTEMS_RENDER_UIDATA_HPP__

#include <Systems/Stores/StoreKey.hpp>
#include <Utils/JsonDeserializer.hpp>
#include <nlohmann/json_fwd.hpp>

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
};

} // namespace Game::Render

#endif // SRC_SYSTEMS_RENDER_UIDATA_HPP__
