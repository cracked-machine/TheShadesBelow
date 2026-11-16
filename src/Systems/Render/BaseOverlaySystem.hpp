#ifndef SRC_SYSTEMS_RENDER_BASEOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_BASEOVERLAYSYSTEM_HPP__

#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Render/UiLayout.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <span>
#include <string_view>

namespace Game::Sys
{

//! @brief Base class for the overlays drawn on top of the game world: HudOverlaySystem, ShopOverlaySystem,
//! PauseOverlaySystem, GrimoireOverlaySystem, DebugOverlaySystem and TooltipOverlaySystem.
//!
//! Each overlay does all of its per-frame work (timers, state, SFML drawing and ImGui windows) in update(). Overlays only
//! add ImGui windows: the ImGui frame itself is begun and rendered once per frame by OverlayArbitrationSystem, which also
//! decides which overlays run and which modal overlay may open.
class BaseOverlaySystem : public RenderSystem
{
public:
  //! @brief polymorphic destructor for derived classes
  ~BaseOverlaySystem() override = default;

  //! @brief Do the overlay's per-frame work: advance its timers and state, and draw it. Called by OverlayArbitrationSystem
  //! between the begin and end of the shared ImGui frame.
  //! @param dt
  virtual void update( sf::Time dt ) = 0;

protected:
  //! @brief Construct a new Base Overlay System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  BaseOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Draw a layout's outline rectangles (panel borders) in screen view coordinates.
  //! @param ui_layout
  void render_outlines( const Render::UiLayout &ui_layout );

  //! @brief The layout's current design-to-screen scale, for hard-coded pixel values (outline widths, font sizes).
  //! @param ui_layout The layout, or nullptr.
  //! @return The scale, or 1 if `ui_layout` is nullptr.
  [[nodiscard]] static float ui_scale( const Render::UiLayout *ui_layout ) { return ui_layout ? ui_layout->scale() : 1.f; }

  //! @brief Scale a design-space character size to the layout's current UI scale.
  //! @param design_size character size in pixels at the layout's design resolution
  //! @param ui_layout The layout, or nullptr for no scaling.
  //! @return the scaled character size, at least 1
  [[nodiscard]] static unsigned int scaled_char_size( unsigned int design_size, const Render::UiLayout *ui_layout );

  //! @brief Horizontal alignment of overlay text about its position.
  enum class TextAlign {
    //! @brief The position is the text's left edge
    LEFT,
    //! @brief The position is the text's centre
    CENTER,
    //! @brief The position is the text's right edge
    RIGHT
  };

  //! @brief Parse a layout ui_label "align" property ("left", "center" or "right"; anything else is left).
  //! @param align the property value
  //! @return TextAlign the alignment
  [[nodiscard]] static TextAlign to_text_align( std::string_view align );

  //! @brief Begin a full-screen, invisible, display-only ImGui window for overlay widgets and text: no inputs, never
  //! focused or brought to the front, zero padding, square frames, the grimoire font. Pair with end_overlay_window().
  //! Text drawn with draw_text() goes into this window's draw list, so it layers with the overlay's other ImGui windows.
  //! @param id the ImGui window id
  static void begin_overlay_window( const char *id );

  //! @brief End the window begun by begin_overlay_window().
  static void end_overlay_window();

  //! @brief Measure overlay text drawn by draw_text() at `size`.
  //! @param text the text
  //! @param size the font size, in pixels
  //! @return sf::Vector2f the text's width and height, in pixels
  [[nodiscard]] static sf::Vector2f measure_text( std::string_view text, float size );

  //! @brief Draw overlay text (res/fonts/tuffy.ttf, see Utils::imgui_text_font) into the current ImGui window's draw
  //! list. ImGui renders after every SFML draw, so the text is never hidden by SFML drawing. Must be called inside an
  //! ImGui window, e.g. begin_overlay_window().
  //! @param text the text
  //! @param size the font size, in pixels
  //! @param pos screen position of the text's top, and its left edge, centre or right edge per `align`
  //! @param fill the text colour
  //! @param align horizontal alignment about `pos`
  //! @param outline the outline colour, drawn round the text when `outline_thickness` is positive
  //! @param outline_thickness the outline width, in pixels
  static void draw_text( std::string_view text, float size, sf::Vector2f pos, sf::Color fill, TextAlign align = TextAlign::LEFT,
                         sf::Color outline = sf::Color::Transparent, float outline_thickness = 0.f );

  //! @brief Draw a display-only selection menu: a centred ImGui window with a title, a sub-title line, a two-column
  //! table of `rows` with a "<" marking the selected row, and a hint line. It takes no mouse input; the selection is
  //! driven by the caller. Call inside the shared ImGui frame.
  //! @param title the menu title
  //! @param subtitle the line under the title, e.g. the cost
  //! @param subtitle_ok false to show the sub-title in red, e.g. when the player can't afford it
  //! @param rows the row labels
  //! @param selected index of the selected row
  //! @param hint the hint line at the bottom, e.g. the controls
  static void render_selection_menu( std::string_view title, std::string_view subtitle, bool subtitle_ok, std::span<const std::string_view> rows,
                                     std::size_t selected, std::string_view hint );

  //! @brief Move a menu selection up/down one row, clamped to [0, row_count).
  //! @param selected the current selection
  //! @param row_count the number of rows
  //! @param next true for the next (lower) row, false for the previous (upper) row
  //! @return std::size_t the new selection
  [[nodiscard]] static std::size_t step_selection( std::size_t selected, std::size_t row_count, bool next );

  //! @brief Side effects of opening a modal overlay (pause, grimoire): cancel any bow draw, then pause the sound effects.
  void on_modal_open();

  //! @brief Side effects of closing a modal overlay: resume the sound effects.
  void on_modal_close();
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_BASEOVERLAYSYSTEM_HPP__
