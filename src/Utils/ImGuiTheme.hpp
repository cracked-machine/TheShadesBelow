#ifndef SRC_UTILS_IMGUITHEME_HPP__
#define SRC_UTILS_IMGUITHEME_HPP__

struct ImFont;

namespace Game::Utils
{

//! @brief Apply the game's custom ImGui look (colors + shape) to the current ImGui context.
//! @note Must be called after ImGui::SFML::Init, since that creates the ImGui context and
//! resets the style to ImGui's default before this can override it.
void apply_imgui_theme();

//! @brief Pixel size the grimoire font is rasterised at, see load_imgui_fonts().
inline constexpr float kImGuiGrimoireFontPx = 20.f;

//! @brief Add the game's font (res/fonts/tuffy.ttf) to the ImGui font atlas, after ImGui's default font so other panels
//! keep their look, and rebuild the atlas texture.
//! @note Must be called after ImGui::SFML::Init (and after every re-Init, which creates a new context), once
//! io.FontGlobalScale has been set: the font cancels that scale so it is drawn at exactly kImGuiGrimoireFontPx.
void load_imgui_fonts();

//! @brief The font added by load_imgui_fonts(), used by the grimoire's tables and tabs.
//! @return The font, or nullptr (ImGui's default font when pushed) if it has not been loaded or failed to load.
ImFont *imgui_grimoire_font();

} // namespace Game::Utils

#endif // SRC_UTILS_IMGUITHEME_HPP__
