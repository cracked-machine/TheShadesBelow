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

void BaseOverlaySystem::on_modal_open()
{
  // the mouse release is swallowed while a modal overlay is open, so abandon any draw now rather than leave it pending.
  // Cancel before pausing the effects so the draw sfx is stopped, not paused and later resumed.
  get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::CANCEL_BOW ) );
  m_sound_bank.pause_effects();
}

void BaseOverlaySystem::on_modal_close() { m_sound_bank.resume_effects(); }

} // namespace Game::Sys
