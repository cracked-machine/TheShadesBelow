#ifndef SRC_UTILS_IMGUITHEME_HPP__
#define SRC_UTILS_IMGUITHEME_HPP__

#include <array>

struct ImFont;

namespace Game::Utils
{

//! @brief Apply the game's custom ImGui look (colors + shape) to the current ImGui context.
//! @note Must be called after ImGui::SFML::Init, since that creates the ImGui context and
//! resets the style to ImGui's default before this can override it.
void apply_imgui_theme();

//! @brief Pixel size the grimoire font is rasterised at, see load_imgui_fonts().
inline constexpr float kImGuiGrimoireFontPx = 20.f;

//! @brief Add the game's font (res/fonts/tuffy.ttf) to the ImGui font atlas, at kImGuiGrimoireFontPx and each kImGuiTextFontPx, after ImGui's default font so other panels
//! keep their look, and rebuild the atlas texture.
//! @note Must be called after ImGui::SFML::Init (and after every re-Init, which creates a new context), once
//! io.FontGlobalScale has been set: the font cancels that scale so it is drawn at exactly kImGuiGrimoireFontPx.
void load_imgui_fonts();

//! @brief Pixel sizes the overlay text font (res/fonts/tuffy.ttf) is rasterised at, see imgui_text_font().
inline constexpr std::array<float, 4> kImGuiTextFontPx{ kImGuiGrimoireFontPx, 40.f, 80.f, 128.f };

//! @brief The overlay text font rasterised closest to `px`: the smallest loaded size that is at least `px` (or the
//! largest), so ImDrawList::AddText( font, px, ... ) only scales it down and stays sharp.
//! @param px the size the text will be drawn at, in pixels
//! @return The font, or ImGui's current font if none was loaded.
ImFont *imgui_text_font( float px );

//! @brief The font added by load_imgui_fonts(), used by the grimoire's tables and tabs.
//! @return The font, or nullptr (ImGui's default font when pushed) if it has not been loaded or failed to load.
ImFont *imgui_grimoire_font();

} // namespace Game::Utils

#endif // SRC_UTILS_IMGUITHEME_HPP__
