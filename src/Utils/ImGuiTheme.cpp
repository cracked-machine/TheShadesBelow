#include <Utils/ImGuiTheme.hpp>

#include <imgui-SFML.h>
#include <array>
#include <imgui.h>
#include <spdlog/spdlog.h>

namespace Game::Utils
{

namespace
{
//! @brief The font added by load_imgui_fonts(). Owned by the current ImGui context's font atlas.
ImFont *s_grimoire_font = nullptr;
//! @brief The game font at each of kImGuiTextFontPx, see imgui_text_font(). Entries are nullptr if loading failed.
std::array<ImFont *, kImGuiTextFontPx.size()> s_text_fonts{};
} // namespace

void apply_imgui_theme()
{
  ImGui::StyleColorsDark();

  ImGuiStyle &style = ImGui::GetStyle();

  // Sharp, angular shapes rather than soft/rounded - fits the game's horror tone better
  // than ImGui's default friendly-rounded look.
  style.WindowRounding = 0.0f;
  style.ChildRounding = 0.0f;
  style.FrameRounding = 2.0f;
  style.PopupRounding = 0.0f;
  style.ScrollbarRounding = 2.0f;
  style.GrabRounding = 2.0f;
  style.TabRounding = 2.0f;

  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
  style.PopupBorderSize = 1.0f;

  style.WindowPadding = ImVec2( 12.0f, 12.0f );
  style.FramePadding = ImVec2( 8.0f, 4.0f );
  style.ItemSpacing = ImVec2( 8.0f, 8.0f );

  ImVec4 *colors = style.Colors;

  // Near-black backgrounds with a muted blood-red accent, cold grey text.
  const ImVec4 kBackground( 0.06f, 0.05f, 0.05f, 0.96f );
  const ImVec4 kBackgroundLight( 0.11f, 0.09f, 0.09f, 1.00f );
  const ImVec4 kBorder( 0.30f, 0.08f, 0.08f, 0.60f );
  const ImVec4 kAccent( 0.55f, 0.10f, 0.10f, 1.00f );
  const ImVec4 kAccentHover( 0.72f, 0.14f, 0.14f, 1.00f );
  const ImVec4 kAccentActive( 0.85f, 0.18f, 0.18f, 1.00f );
  const ImVec4 kText( 0.82f, 0.80f, 0.80f, 1.00f );
  const ImVec4 kTextDisabled( 0.45f, 0.42f, 0.42f, 1.00f );

  colors[ImGuiCol_Text] = kText;
  colors[ImGuiCol_TextDisabled] = kTextDisabled;
  colors[ImGuiCol_WindowBg] = kBackground;
  colors[ImGuiCol_ChildBg] = kBackground;
  colors[ImGuiCol_PopupBg] = kBackground;
  colors[ImGuiCol_Border] = kBorder;

  colors[ImGuiCol_FrameBg] = kBackgroundLight;
  colors[ImGuiCol_FrameBgHovered] = kAccent;
  colors[ImGuiCol_FrameBgActive] = kAccentHover;

  colors[ImGuiCol_TitleBg] = kBackground;
  colors[ImGuiCol_TitleBgActive] = kAccent;
  colors[ImGuiCol_TitleBgCollapsed] = kBackground;

  colors[ImGuiCol_CheckMark] = kAccentActive;
  colors[ImGuiCol_SliderGrab] = kAccent;
  colors[ImGuiCol_SliderGrabActive] = kAccentActive;

  colors[ImGuiCol_Button] = kBackgroundLight;
  colors[ImGuiCol_ButtonHovered] = kAccent;
  colors[ImGuiCol_ButtonActive] = kAccentHover;

  colors[ImGuiCol_Header] = kAccent;
  colors[ImGuiCol_HeaderHovered] = kAccentHover;
  colors[ImGuiCol_HeaderActive] = kAccentActive;

  colors[ImGuiCol_Separator] = kBorder;
  colors[ImGuiCol_SeparatorHovered] = kAccentHover;
  colors[ImGuiCol_SeparatorActive] = kAccentActive;

  colors[ImGuiCol_ScrollbarBg] = kBackground;
  colors[ImGuiCol_ScrollbarGrab] = kBackgroundLight;
  colors[ImGuiCol_ScrollbarGrabHovered] = kAccent;
  colors[ImGuiCol_ScrollbarGrabActive] = kAccentHover;

  colors[ImGuiCol_Tab] = kBackgroundLight;
  colors[ImGuiCol_TabHovered] = kAccentHover;
  colors[ImGuiCol_TabActive] = kAccent;
}

void load_imgui_fonts()
{
  ImGuiIO &io = ImGui::GetIO();
  s_grimoire_font = io.Fonts->AddFontFromFileTTF( "res/fonts/tuffy.ttf", kImGuiGrimoireFontPx );
  if ( not s_grimoire_font )
  {
    SPDLOG_ERROR( "Failed to load the ImGui grimoire font res/fonts/tuffy.ttf" );
    return;
  }
  // cancel the global scale applied to every font, so this one draws at exactly the size it was rasterised at
  s_grimoire_font->Scale = 1.f / io.FontGlobalScale;

  // the overlay text font at a few sizes: ImGui 1.91 rasterises a font at one size, so large text drawn from a small
  // font would be blurry. The grimoire font doubles as the smallest size.
  for ( std::size_t idx = 0; idx < kImGuiTextFontPx.size(); ++idx )
  {
    if ( kImGuiTextFontPx.at( idx ) == kImGuiGrimoireFontPx )
    {
      s_text_fonts.at( idx ) = s_grimoire_font;
      continue;
    }
    s_text_fonts.at( idx ) = io.Fonts->AddFontFromFileTTF( "res/fonts/tuffy.ttf", kImGuiTextFontPx.at( idx ) );
    if ( not s_text_fonts.at( idx ) )
    {
      SPDLOG_ERROR( "Failed to load res/fonts/tuffy.ttf at {}px for ImGui overlay text", kImGuiTextFontPx.at( idx ) );
      continue;
    }
    s_text_fonts.at( idx )->Scale = 1.f / io.FontGlobalScale;
  }
  if ( not ImGui::SFML::UpdateFontTexture() ) { SPDLOG_ERROR( "Failed to update the ImGui font texture" ); }
}

ImFont *imgui_grimoire_font() { return s_grimoire_font; }

ImFont *imgui_text_font( float px )
{
  // the smallest loaded size at least as big as `px`, so the text is only ever scaled down
  ImFont *best = nullptr;
  for ( std::size_t idx = 0; idx < kImGuiTextFontPx.size(); ++idx )
  {
    if ( not s_text_fonts.at( idx ) ) continue;
    best = s_text_fonts.at( idx );
    if ( kImGuiTextFontPx.at( idx ) >= px ) break;
  }
  return best ? best : ImGui::GetFont();
}

} // namespace Game::Utils
