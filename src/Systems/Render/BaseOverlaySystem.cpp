#include <Audio/SoundBank.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Systems/Render/BaseOverlaySystem.hpp>

#include <SFML/Graphics/RectangleShape.hpp>
#include <Utils/ImGuiTheme.hpp>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <string>
#include <tuple>

namespace Game::Sys
{

namespace
{
//! @brief Window flags for the overlays' display-only ImGui windows: they never take the mouse from the game or come in
//! front of other overlays.
constexpr ImGuiWindowFlags kOverlayWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove |
                                                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing |
                                                 ImGuiWindowFlags_NoScrollbar;

ImU32 to_imu32( sf::Color colour ) { return IM_COL32( colour.r, colour.g, colour.b, colour.a ); }
} // namespace

BaseOverlaySystem::BaseOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : RenderSystem( reg, window, sound_bank )
{
}

BaseOverlaySystem::TextAlign BaseOverlaySystem::to_text_align( std::string_view align )
{
  if ( align == "center" ) return TextAlign::CENTER;
  if ( align == "right" ) return TextAlign::RIGHT;
  return TextAlign::LEFT;
}

void BaseOverlaySystem::begin_overlay_window( const char *id )
{
  ImGui::SetNextWindowPos( ImVec2( 0.f, 0.f ) );
  ImGui::SetNextWindowSize( ImGui::GetIO().DisplaySize );
  ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.f, 0.f ) );
  ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 0.f );
  ImGui::PushFont( Utils::imgui_grimoire_font() );
  // the return value only says whether the window is collapsed or clipped; an invisible full-screen window never is
  std::ignore = ImGui::Begin( id, nullptr, kOverlayWindowFlags );
}

void BaseOverlaySystem::end_overlay_window()
{
  ImGui::End();
  ImGui::PopFont();
  ImGui::PopStyleVar( 2 );
}

sf::Vector2f BaseOverlaySystem::measure_text( std::string_view text, float size )
{
  const ImVec2 measured = Utils::imgui_text_font( size )->CalcTextSizeA( size, FLT_MAX, 0.f, text.data(), text.data() + text.size() );
  return { measured.x, measured.y };
}

void BaseOverlaySystem::draw_text( std::string_view text, float size, sf::Vector2f pos, sf::Color fill, TextAlign align, sf::Color outline,
                                   float outline_thickness )
{
  if ( text.empty() ) return;
  ImFont *font = Utils::imgui_text_font( size );
  if ( align == TextAlign::CENTER ) { pos.x -= measure_text( text, size ).x / 2.f; }
  else if ( align == TextAlign::RIGHT ) { pos.x -= measure_text( text, size ).x; }

  auto &draw_list = *ImGui::GetWindowDrawList();
  const char *begin = text.data();
  const char *end = text.data() + text.size();
  // the outline: the text in the outline colour at the eight offsets round it, then the text itself on top
  if ( outline_thickness > 0.f and outline.a > 0 )
  {
    constexpr std::array<std::pair<float, float>, 8> kOffsets{
        { { -1.f, -1.f }, { 0.f, -1.f }, { 1.f, -1.f }, { -1.f, 0.f }, { 1.f, 0.f }, { -1.f, 1.f }, { 0.f, 1.f }, { 1.f, 1.f } } };
    for ( const auto &[dx, dy] : kOffsets )
    {
      draw_list.AddText( font, size, ImVec2( pos.x + ( dx * outline_thickness ), pos.y + ( dy * outline_thickness ) ), to_imu32( outline ), begin,
                         end );
    }
  }
  draw_list.AddText( font, size, ImVec2( pos.x, pos.y ), to_imu32( fill ), begin, end );
}

void BaseOverlaySystem::render_outlines( const Render::UiLayout &ui_layout )
{
  sf::RectangleShape rect;
  for ( const auto &outline : ui_layout.m_outlines )
  {
    rect.setSize( outline.rect.size );
    rect.setPosition( outline.rect.position );
    rect.setFillColor( outline.fill_color );
    rect.setOutlineColor( outline.line_color );
    rect.setOutlineThickness( static_cast<float>( outline.line_thickness ) );
    rect.setRotation( outline.rotation );
    draw_screen( rect );
  }
}

unsigned int BaseOverlaySystem::scaled_char_size( unsigned int design_size, const Render::UiLayout *ui_layout )
{
  return std::max( 1u, static_cast<unsigned int>( std::lround( static_cast<float>( design_size ) * ui_scale( ui_layout ) ) ) );
}

void BaseOverlaySystem::render_selection_menu( std::string_view title, std::string_view subtitle, bool subtitle_ok,
                                               std::span<const std::string_view> rows, std::size_t selected, std::string_view hint )
{
  constexpr float kMenuTopFraction = 0.2f;
  // one size up from the grimoire font
  constexpr float kMenuFontPx = Utils::kImGuiTextFontPx.at( 1 );
  constexpr float kLabelColumnWidth = 220.f;
  constexpr float kMarkerColumnWidth = 40.f;
  const ImVec4 kTitleColour( 1.f, 0.85f, 0.4f, 1.f );
  const ImVec4 kWarningColour( 1.f, 0.3f, 0.3f, 1.f );
  const ImVec4 kHintColour( 0.75f, 0.75f, 0.75f, 1.f );

  const ImVec2 display_size = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowPos( ImVec2( display_size.x * 0.5f, display_size.y * kMenuTopFraction ), ImGuiCond_Always, ImVec2( 0.5f, 0.f ) );
  ImGui::SetNextWindowBgAlpha( 0.75f );
  // display-only: the caller drives the selection, so it never takes the mouse from the game
  constexpr ImGuiWindowFlags kWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings |
                                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
                                            ImGuiWindowFlags_NoNav;
  ImGui::PushFont( Utils::imgui_text_font( kMenuFontPx ) );
  if ( ImGui::Begin( "##selection_menu", nullptr, kWindowFlags ) )
  {
    ImGui::TextColored( kTitleColour, "%s", std::string( title ).c_str() );
    ImGui::TextColored( subtitle_ok ? kHintColour : kWarningColour, "%s", std::string( subtitle ).c_str() );
    ImGui::Separator();

    if ( ImGui::BeginTable( "##selection_menu_table", 2, ImGuiTableFlags_SizingFixedFit ) )
    {
      ImGui::TableSetupColumn( "label", ImGuiTableColumnFlags_WidthFixed, kLabelColumnWidth );
      ImGui::TableSetupColumn( "marker", ImGuiTableColumnFlags_WidthFixed, kMarkerColumnWidth );
      for ( std::size_t row = 0; row < rows.size(); ++row )
      {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );
        ImGui::TextUnformatted( rows[row].data(), rows[row].data() + rows[row].size() );
        ImGui::TableSetColumnIndex( 1 );
        if ( row == selected ) { ImGui::TextUnformatted( "<" ); }
      }
      ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::TextColored( kHintColour, "%s", std::string( hint ).c_str() );
  }
  ImGui::End();
  ImGui::PopFont();
}

std::size_t BaseOverlaySystem::step_selection( std::size_t selected, std::size_t row_count, bool next )
{
  if ( next ) return selected + 1 < row_count ? selected + 1 : selected;
  return selected > 0 ? selected - 1 : selected;
}

void BaseOverlaySystem::cancel_bow_draw()
{
  get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::CANCEL_BOW ) );
}

void BaseOverlaySystem::on_modal_open()
{
  // cancel before pausing the effects so the draw sfx is stopped, not paused and later resumed
  cancel_bow_draw();
  m_sound_bank.pause_effects();
}

void BaseOverlaySystem::on_modal_close() { m_sound_bank.resume_effects(); }

} // namespace Game::Sys
