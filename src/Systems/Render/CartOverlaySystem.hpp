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
//! only: it takes no input and does not pause gameplay. The same row is drawn on the level complete screen by
//! render_level_tally(), in a "Cart" section above an "Inventory" section of icon rows, highlighting each element as it is
//! scored.
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

  //! @brief Construct a new Cart Overlay System object. Loads the slot and panel layouts from res/ui/barrel_slot.json
  //! and res/ui/barrel_panel.json.
  //! @param reg
  //! @param window
  //! @param sound_bank
  CartOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Draw the barrel slot row, if a cart's overlay is open.
  //! @param dt
  void update( sf::Time dt ) override;

  //! @brief Draw the level complete tally in its own ImGui frame: a "Cart" section with the barrel row, and beneath it
  //! an "Inventory" section with `icon_rows`, one under the other. A section with nothing to show is left out. Call
  //! after the SFML parts of the screen are drawn, as the ImGui frame is rendered over them.
  //! @param dt
  //! @param barrels The barrels, left to right.
  //! @param barrel_progress Tally position in the barrel row, or std::nullopt before counting starts.
  //! @param icon_rows The inventory rows, top to bottom. Empty rows are skipped.
  //! @param top_center Window pixel position of the top centre of the tally.
  void render_level_tally( sf::Time dt, std::span<const Cmp::Cart::Barrel> barrels, std::optional<BarrelProgress> barrel_progress,
                           std::span<const IconRow> icon_rows, sf::Vector2f top_center );

private:
  //! @brief Draw a row of barrel slots, with the filled part sprites. Must be called inside an ImGui window.
  //! @param barrels The barrels, left to right.
  //! @param row_center Window pixel position of the row's centre.
  //! @param progress Tally position to dim/highlight, or std::nullopt for neither.
  //! @return The panel's height in window pixels.
  float render_barrel_row( std::span<const Cmp::Cart::Barrel> barrels, sf::Vector2f row_center, std::optional<BarrelProgress> progress );

  //! @brief Draw the inventory section's rows of icons inside a panel styled like the barrel row's. Must be called inside an
  //! ImGui window.
  //! @param icon_rows The rows, top to bottom. Empty rows are skipped.
  //! @param top_center Window pixel position of the panel's top centre.
  //! @return The panel's height in window pixels.
  float render_icon_rows( std::span<const IconRow> icon_rows, sf::Vector2f top_center );

  //! @brief Draw the res/ui/barrel_panel.json panel, moved and resized to `rect`. Must be called inside an ImGui window.
  //! @param rect Window pixel rectangle of the panel.
  //! @param scale UI scale, for the border thickness.
  void render_panel( const sf::FloatRect &rect, float scale );

  //! @brief Draw a section caption, centred above a section of the tally. Must be called inside an ImGui window.
  //! @param caption The caption text.
  //! @param top_center Window pixel position of the caption's top centre.
  //! @param scale UI scale, for the font size.
  //! @return The caption's height plus the gap beneath it, in window pixels.
  static float render_caption( std::string_view caption, sf::Vector2f top_center, float scale );

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

  //! @brief Name of the ui_outline in res/ui/barrel_panel.json that is resized to fit the row.
  static constexpr const char *kPanelOutlineName = "barrel_panel";
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_CARTOVERLAYSYSTEM_HPP__
