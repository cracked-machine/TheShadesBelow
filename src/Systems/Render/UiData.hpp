#ifndef SRC_SYSTEMS_RENDER_UIDATA_HPP__
#define SRC_SYSTEMS_RENDER_UIDATA_HPP__

#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/JsonDeserializer.hpp>
#include <nlohmann/json_fwd.hpp>

#include <string>
#include <utility>
#include <vector>

namespace Game::Render
{

//! @brief Loads and holds UI layout data (outlines, labels, texts, meters and icons) deserialized from a Tiled JSON UI map file.
//!
//! The Tiled map is authored in "design space": its pixel size (width * tilewidth, height * tileheight, e.g. 1920x1080).
//! The JSON is never modified; instead sync_to() maps the design-space elements onto the current display resolution with
//! a uniform scale (so sprites and text keep their aspect ratio) and centres the result. The public element lists always
//! hold the screen-space (scaled) values; until sync_to() is first called they equal the design-space values.
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
    //! @brief Scale factor applied to the icon sprite. Authored as an integer in Tiled, then multiplied by the UI scale.
    float scale;
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

  //! @brief Rescale the layout for `display_res`. Cheap no-op when the layout is already scaled for that resolution and
  //! anchor, so it can be called every frame (e.g. at the top of a render pass) to pick up display resolution changes.
  //! @param display_res the target (window) resolution in pixels
  //! @param anchor_offset an extra screen-space translation applied after scaling and centring, e.g. to move a panel
  //! to the mouse cursor. Defaults to none.
  void sync_to( sf::Vector2u display_res, sf::Vector2f anchor_offset = { 0.f, 0.f } );

  //! @brief The uniform design-to-screen scale currently applied (1.0 until sync_to() is called).
  //! Use it to scale hard-coded pixel values (font sizes, offsets) drawn alongside this layout.
  [[nodiscard]] float scale() const { return m_scale; }

  //! @brief The screen-space offset currently applied after scaling, i.e. the letterbox margin that centres the layout.
  [[nodiscard]] sf::Vector2f offset() const { return m_offset; }

  //! @brief The anchor offset currently applied after scaling and centring, see sync_to().
  [[nodiscard]] sf::Vector2f anchor_offset() const { return m_anchor_offset; }

  //! @brief The layout's design size in pixels, read from the Tiled map (width * tilewidth, height * tileheight).
  [[nodiscard]] sf::Vector2f design_size() const { return m_design_size; }

  //! @brief Uniform scale that fits `design` inside `display_res` without distortion.
  //! @param display_res the target (window) resolution in pixels
  //! @param design the design-space size in pixels, defaults to kDefaultDesignSize
  //! @return min( display_res.x / design.x, display_res.y / design.y )
  [[nodiscard]] static float ui_scale( sf::Vector2u display_res, sf::Vector2f design = kDefaultDesignSize );

  //! @brief Design size used when a map does not define its own size, and by ui_scale() for code without a layout.
  static constexpr sf::Vector2f kDefaultDesignSize{ 1920.f, 1080.f };

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
  //! @brief Design-space size of the layout, see design_size().
  sf::Vector2f m_design_size{ kDefaultDesignSize };
  //! @brief Resolution the public element lists are currently scaled for ({0,0} = unscaled design space).
  sf::Vector2u m_scaled_for{ 0u, 0u };
  //! @brief See scale().
  float m_scale{ 1.f };
  //! @brief See offset().
  sf::Vector2f m_offset{ 0.f, 0.f };
  //! @brief The anchor offset the public element lists are currently translated by, see sync_to().
  sf::Vector2f m_anchor_offset{ 0.f, 0.f };

  //! @brief Design-space copies of the parsed elements, as authored in the Tiled map. sync_to() rebuilds the public lists from these.
  std::vector<Outline> m_design_outlines;
  std::vector<Label> m_design_labels;
  std::vector<Text> m_design_texts;
  std::vector<Meter> m_design_meters;
  std::vector<Icon> m_design_icons;
  std::vector<Table> m_design_tables;
  std::vector<Tab> m_design_tabs;

  //! @brief Parse a ui_table "rows" property, e.g. "burn_action=Burn, destroy_action=Chop".
  //! @param rows The property value.
  //! @return The (key, caption) pairs, in order. An entry without '=' uses the key as its caption.
  static std::vector<std::pair<std::string, std::string>> parse_table_rows( const std::string &rows );
};

} // namespace Game::Render

#endif // SRC_SYSTEMS_RENDER_UIDATA_HPP__
