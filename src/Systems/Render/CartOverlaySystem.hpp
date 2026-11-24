#ifndef SRC_SYSTEMS_RENDER_CARTOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_CARTOVERLAYSYSTEM_HPP__

#include <Components/Cart/Barrel.hpp>
#include <Systems/Render/BaseOverlaySystem.hpp>
#include <Systems/Render/UiLayout.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <SFML/System/Time.hpp>

#include <memory>
#include <optional>
#include <span>

namespace Game::Sys
{

//! @brief Draws a row, centred over the cart sprite, with one res/ui/barrel_slot.json slot per Cmp::Cart::Barrel while a
//! cart has Cmp::Cart::BarrelOverlayOpen. Every slot draws its part outlines, and the part sprites of its filled flags. Display
//! only: it takes no input and does not pause gameplay. The same row is drawn on the level complete screen by
//! render_level_tally(), highlighting each part as it is scored.
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

  //! @brief Construct a new Cart Overlay System object. Loads the slot and panel layouts from res/ui/barrel_slot.json
  //! and res/ui/barrel_panel.json.
  //! @param reg
  //! @param window
  //! @param sound_bank
  CartOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Draw the barrel slot row, if a cart's overlay is open.
  //! @param dt
  void update( sf::Time dt ) override;

  //! @brief Draw the level complete tally in its own ImGui frame: the barrel row, with a row of `icon_sprite` icons
  //! beneath it. Call after the SFML parts of the screen are drawn, as the ImGui frame is rendered over them.
  //! @param dt
  //! @param barrels The barrels, left to right.
  //! @param barrel_progress Tally position in the barrel row, or std::nullopt before counting starts.
  //! @param icon_sprite Sprite for the icon row.
  //! @param icon_count Number of icons in the row.
  //! @param icon_progress Tally position in the icon row, or std::nullopt before counting reaches it.
  //! @param top_center Window pixel position of the top centre of the barrel row.
  void render_level_tally( sf::Time dt, std::span<const Cmp::Cart::Barrel> barrels, std::optional<BarrelProgress> barrel_progress,
                           const Sys::SpriteKey &icon_sprite, std::size_t icon_count, std::optional<IconProgress> icon_progress,
                           sf::Vector2f top_center );

private:
  //! @brief Draw a row of barrel slots, with the filled part sprites. Must be called inside an ImGui window.
  //! @param barrels The barrels, left to right.
  //! @param row_center Window pixel position of the row's centre.
  //! @param progress Tally position to dim/highlight, or std::nullopt for neither.
  //! @return The panel's height in window pixels.
  float render_barrel_row( std::span<const Cmp::Cart::Barrel> barrels, sf::Vector2f row_center, std::optional<BarrelProgress> progress );

  //! @brief Dim an already counted element of the tally.
  //! @param rect Window pixel rectangle to dim.
  static void dim_rect( const sf::FloatRect &rect );

  //! @brief Highlight the element of the tally being counted.
  //! @param rect Window pixel rectangle to highlight.
  //! @param scale UI scale, for the border thickness.
  static void highlight_rect( const sf::FloatRect &rect, float scale );

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
