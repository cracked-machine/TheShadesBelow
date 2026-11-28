#ifndef SRC_SYSTEMS_RENDER_CARTOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_CARTOVERLAYSYSTEM_HPP__

#include <Components/Cart/Barrel.hpp>
#include <Systems/Render/BaseOverlaySystem.hpp>
#include <Systems/Render/UiLayout.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Time.hpp>

#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace Game::Sys
{

//! @brief Draws a row, centred over the cart sprite, with one res/ui/barrel_slot.json slot per Cmp::Cart::Barrel while a
//! cart has Cmp::Cart::BarrelOverlayOpen. Every slot draws its part outlines, and the part sprites of its filled flags. Display
//! only: it takes no input and does not pause gameplay. It also draws the whole level complete screen
//! (render_level_tally()), laid out by res/ui/levelcomplete.json: the same row in a "Cart" section, an "Inventory" section
//! of icon rows, the item on the table, the tally table and the screen text, highlighting each element as it is scored.
class CartOverlaySystem : public BaseOverlaySystem
{
public:
  //! @brief How far the level complete tally has got through a barrel row. Everything before the current position is
  //! counted (dimmed); the current position is highlighted.
  struct BarrelProgress
  {
    //! @brief The barrel being counted. Barrels before it are counted; barrels.size() means every barrel is counted.
    std::size_t barrel{ 0 };
    //! @brief Index into Cmp::Cart::Barrel::kParts of the part being counted. Earlier parts are counted.
    std::size_t part{ 0 };
    //! @brief The full-barrel bonus: highlight the whole slot. Every part of the barrel is counted, `part` is ignored.
    bool whole_barrel{ false };
  };

  //! @brief How far the level complete tally has got through a row of icons (e.g. preserved cats).
  struct IconProgress
  {
    //! @brief The icon being counted. Icons before it are counted; the icon count means every icon is counted.
    std::size_t current{ 0 };
  };

  //! @brief Highlight colour of the element being counted (barrels, preserved cats).
  static constexpr sf::Color kTallyHighlight{ 255, 215, 80 };
  //! @brief Highlight colour of a spare cadaver part being counted, setting it apart from the barrelled parts.
  static constexpr sf::Color kSpareHighlight{ 255, 110, 60 };

  //! @brief One row of icons in the level complete tally's inventory section.
  struct IconRow
  {
    //! @brief The icon sprites, left to right.
    std::span<const Sys::SpriteKey> sprites;
    //! @brief Tally position in the row, or std::nullopt before counting reaches it.
    std::optional<IconProgress> progress;
    //! @brief Colour of the highlight on the icon being counted.
    sf::Color highlight{ kTallyHighlight };
  };

  //! @brief What the level complete tally has counted so far. The keys of the res/ui/levelcomplete.json "tally_table"
  //! rows name these fields, see tally_value().
  struct VictoryTally
  {
    //! @brief Wealth added so far by the tally.
    int score_wealth{ 0 };
    //! @brief Cadaver parts counted.
    int parts{ 0 };
    //! @brief Full barrels counted.
    int full_barrels{ 0 };
    //! @brief Preserved cats counted.
    int cats{ 0 };
    //! @brief Spare cadaver parts (in the inventory, not a barrel) counted.
    int spare_parts{ 0 };
  };

  //! @brief Construct a new Cart Overlay System object. Loads the slot and panel layouts from res/ui/barrel_slot.json
  //! and res/ui/barrel_panel.json, and the level complete layout from res/ui/levelcomplete.json.
  //! @throws std::runtime_error if the level complete layout is missing an element, or has a "tally_table" row key that
  //! is not a VictoryTally field.
  //! @param reg
  //! @param window
  //! @param sound_bank
  CartOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Draw the barrel slot row, if a cart's overlay is open.
  //! @param dt
  void update( sf::Time dt ) override;

  //! @brief Draw the level complete screen in its own ImGui frame, laid out by res/ui/levelcomplete.json: the title, the
  //! continue prompt, the tally table, the player's wealth, the item on the table, a "Cart" section with the barrel row
  //! and an "Inventory" section with `icon_rows` (or the player's empty inventory slots when every row is empty). The
  //! "Cart" section is left out when there are no barrels. Call after the SFML parts of
  //! the screen are drawn, as the ImGui frame is rendered over them.
  //! @param dt
  //! @param barrels The barrels, left to right.
  //! @param barrel_progress Tally position in the barrel row, or std::nullopt before counting starts.
  //! @param icon_rows The inventory rows, top to bottom. Empty rows are skipped.
  //! @param show_continue Whether to draw the continue prompt this frame (the caller flashes it once everything is counted).
  //! @param tally What the tally has counted so far.
  //! @param table_item The item on the table, or std::nullopt for an empty table.
  //! @param table_highlight Highlight colour of `table_item`.
  void render_level_tally( sf::Time dt, std::span<const Cmp::Cart::Barrel> barrels, std::optional<BarrelProgress> barrel_progress,
                           std::span<const IconRow> icon_rows, bool show_continue, const VictoryTally &tally,
                           const std::optional<Sys::SpriteKey> &table_item, sf::Color table_highlight );

private:
  //! @brief Draw a row of barrel slots, with the filled part sprites. Must be called inside an ImGui window.
  //! @param barrels The barrels, left to right.
  //! @param row_center Window pixel position of the row's centre.
  //! @param progress Tally position to dim/highlight, or std::nullopt for neither.
  //! @return The panel's height in window pixels.
  float render_barrel_row( std::span<const Cmp::Cart::Barrel> barrels, sf::Vector2f row_center, std::optional<BarrelProgress> progress );

  //! @brief Window pixel size of the panel render_barrel_row() draws round `barrel_count` slots.
  //! @param barrel_count Number of barrels in the row.
  //! @param scale UI scale.
  [[nodiscard]] sf::Vector2f barrel_row_panel_size( std::size_t barrel_count, float scale ) const;

  //! @brief Draw one barrel slot (res/ui/barrel_slot.json) with its filled part sprites. Must be called inside an ImGui
  //! window.
  //! @param barrel_cmp The barrel.
  //! @param barrel_idx The barrel's index in the tally, compared with `progress`.
  //! @param slot_origin Window pixel position of the slot's top-left.
  //! @param scale Slot scale.
  //! @param progress Tally position to dim/highlight, or std::nullopt for neither.
  void render_barrel_slot( const Cmp::Cart::Barrel &barrel_cmp, std::size_t barrel_idx, sf::Vector2f slot_origin, float scale,
                           std::optional<BarrelProgress> progress );

  //! @brief Draw the level complete title, continue prompt and wealth labels. Must be called inside an ImGui window.
  //! @param show_continue Whether to draw the continue prompt this frame.
  void render_level_labels( bool show_continue );

  //! @brief Draw the level complete "tally_table": one row per layout row, its caption then its VictoryTally value. Must
  //! be called inside an ImGui window.
  //! @param tally What the tally has counted so far.
  void render_tally_table( const VictoryTally &tally );

  //! @brief Draw the item on the table fitted and centred in the "table_icon" rect, highlighted. Must be called inside an
  //! ImGui window.
  //! @param item The item.
  //! @param highlight Highlight colour.
  void render_table_item( const Sys::SpriteKey &item, sf::Color highlight );

  //! @brief The VictoryTally field named by a "tally_table" row key.
  //! @param tally The tally.
  //! @param key The row key, e.g. "parts".
  //! @return The field's value, or std::nullopt if `key` names no field.
  [[nodiscard]] static std::optional<int> tally_value( const VictoryTally &tally, std::string_view key );

  //! @brief Draw the inventory section's rows of icons inside a panel styled like the barrel row's. Must be called inside an
  //! ImGui window.
  //! @param icon_rows The rows, top to bottom. Empty rows are skipped.
  //! @param top_left Window pixel position of the panel's top-left. Rows are left-aligned inside the panel.
  //! @return The panel's height in window pixels.
  float render_icon_rows( std::span<const IconRow> icon_rows, sf::Vector2f top_left );

  //! @brief Draw a row of `slot_count` empty slots inside a panel styled like the barrel row's, for when the inventory
  //! holds nothing to count. Must be called inside an ImGui window.
  //! @param slot_count Number of empty slots, e.g. the player's inventory slot count.
  //! @param top_left Window pixel position of the panel's top-left.
  //! @return The panel's height in window pixels.
  float render_empty_slots( std::size_t slot_count, sf::Vector2f top_left );

  //! @brief Draw the res/ui/barrel_panel.json panel, moved and resized to `rect`. Must be called inside an ImGui window.
  //! @param rect Window pixel rectangle of the panel.
  //! @param scale UI scale, for the border thickness.
  void render_panel( const sf::FloatRect &rect, float scale );

  //! @brief Draw a section caption, left-aligned above a section of the tally. Must be called inside an ImGui window.
  //! @param caption The caption text.
  //! @param top_left Window pixel position of the caption's top-left.
  //! @param scale UI scale, for the font size.
  //! @return The caption's height plus the gap beneath it, in window pixels.
  static float render_caption( std::string_view caption, sf::Vector2f top_left, float scale );

  //! @brief Dim an already counted element of the tally.
  //! @param rect Window pixel rectangle to dim.
  static void dim_rect( const sf::FloatRect &rect );

  //! @brief Highlight the element of the tally being counted.
  //! @param rect Window pixel rectangle to highlight.
  //! @param scale UI scale, for the border thickness.
  //! @param colour Highlight colour; its alpha is ignored.
  static void highlight_rect( const sf::FloatRect &rect, float scale, sf::Color colour = kTallyHighlight );

  //! @brief The layout of one barrel slot, moved to each slot's position as it is drawn.
  std::unique_ptr<Render::UiLayout> m_slot_ui_layout;

  //! @brief The border and background behind the row (res/ui/barrel_panel.json), authored around one slot. The padding
  //! between its size and the slot's is kept around the whole row.
  std::unique_ptr<Render::UiLayout> m_panel_ui_layout;

  //! @brief The level complete screen's layout (res/ui/levelcomplete.json), synced to the window each frame.
  std::unique_ptr<Render::UiLayout> m_level_complete_ui_layout;

  //! @brief Name of the ui_outline in res/ui/barrel_panel.json that is resized to fit the row.
  static constexpr const char *kPanelOutlineName = "barrel_panel";

  //! @brief Sprite scale of the level complete inventory icons, before UI scaling.
  static constexpr float kInventoryIconScale = 2.f;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_CARTOVERLAYSYSTEM_HPP__
