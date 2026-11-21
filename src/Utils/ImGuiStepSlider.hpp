#ifndef SRC_UTILS_IMGUISTEPSLIDER_HPP__
#define SRC_UTILS_IMGUISTEPSLIDER_HPP__

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace Game::Utils
{

//! @brief Multiplier applied to the step when Ctrl is held while clicking a -/+ button.
inline constexpr int kImGuiStepCtrlMultiplier = 10;

//! @brief The smallest increment a printf-style float format displays, e.g. "%.1f" -> 0.1, "%.0f" -> 1, "%.3f pixels" -> 0.001.
//! @param fmt the slider's format string
//! @return 10^-precision, or 0.001 when the format has no explicit precision.
inline float imgui_format_step( const char *fmt )
{
  if ( fmt == nullptr ) return 0.001f;
  const std::string_view sv( fmt );
  const auto pct = sv.find( '%' );
  if ( pct == std::string_view::npos ) return 0.001f;
  const auto dot = sv.find( '.', pct );
  if ( dot == std::string_view::npos ) return 0.001f;

  int precision = 0;
  bool has_digits = false;
  for ( auto i = dot + 1; i < sv.size() and sv[i] >= '0' and sv[i] <= '9'; ++i )
  {
    precision = precision * 10 + ( sv[i] - '0' );
    has_digits = true;
  }
  if ( not has_digits ) return 1.f; // "%.f" means precision 0
  return std::pow( 10.f, -static_cast<float>( std::min( precision, 6 ) ) );
}

namespace detail
{

template <typename T>
constexpr ImGuiDataType imgui_data_type()
{
  if constexpr ( std::is_same_v<T, float> ) return ImGuiDataType_Float;
  else if constexpr ( std::is_same_v<T, double> ) return ImGuiDataType_Double;
  else if constexpr ( std::is_same_v<T, std::uint8_t> ) return ImGuiDataType_U8;
  else if constexpr ( std::is_same_v<T, std::uint16_t> ) return ImGuiDataType_U16;
  else if constexpr ( std::is_same_v<T, std::uint32_t> ) return ImGuiDataType_U32;
  else if constexpr ( std::is_same_v<T, std::uint64_t> ) return ImGuiDataType_U64;
  else if constexpr ( std::is_same_v<T, std::int8_t> ) return ImGuiDataType_S8;
  else if constexpr ( std::is_same_v<T, std::int16_t> ) return ImGuiDataType_S16;
  else if constexpr ( std::is_same_v<T, std::int32_t> ) return ImGuiDataType_S32;
  else if constexpr ( std::is_same_v<T, std::int64_t> ) return ImGuiDataType_S64;
  else static_assert( sizeof( T ) == 0, "imgui_step_slider: unsupported type" );
}

//! @brief Move `value` one step towards `max` (up) or `min` (down), clamped to the range without integer wrap-around.
//! Floats are snapped to a multiple of `step` so repeated presses don't accumulate rounding error.
template <typename T>
T step_value( T value, T min, T max, T step, bool up )
{
  if constexpr ( std::is_floating_point_v<T> )
  {
    T next = up ? value + step : value - step;
    next = std::round( next / step ) * step;
    return std::clamp( next, min, max );
  }
  else
  {
    if ( up ) return ( value >= max or max - value <= step ) ? max : static_cast<T>( value + step );
    return ( value <= min or value - min <= step ) ? min : static_cast<T>( value - step );
  }
}

} // namespace detail

//! @brief A slider followed by -/+ buttons, laid out within `total_width` and without a visible label.
//! @details The buttons repeat while held and step by kImGuiStepCtrlMultiplier x `step` while Ctrl is held.
//! Ctrl+click on the slider still allows typing an exact value.
//! @param id unique ImGui id for this widget (the text is not rendered)
//! @return true if the value changed this frame
template <typename T>
bool imgui_step_slider_body( const char *id, T *value, const T *min, const T *max, const char *fmt, T step, float total_width )
{
  const ImGuiStyle &style = ImGui::GetStyle();
  const float button_size = ImGui::GetFrameHeight();
  const float buttons_width = 2.f * ( button_size + style.ItemInnerSpacing.x );

  ImGui::PushID( id );
  ImGui::SetNextItemWidth( std::max( 1.f, total_width - buttons_width ) );
  bool changed = ImGui::SliderScalar( "##slider", detail::imgui_data_type<T>(), value, min, max, fmt );

  const T applied_step = ImGui::GetIO().KeyCtrl ? static_cast<T>( step * kImGuiStepCtrlMultiplier ) : step;
  ImGui::PushItemFlag( ImGuiItemFlags_ButtonRepeat, true );
  ImGui::SameLine( 0.f, style.ItemInnerSpacing.x );
  if ( ImGui::Button( "-", ImVec2( button_size, button_size ) ) )
  {
    *value = detail::step_value( *value, *min, *max, applied_step, false );
    changed = true;
  }
  ImGui::SameLine( 0.f, style.ItemInnerSpacing.x );
  if ( ImGui::Button( "+", ImVec2( button_size, button_size ) ) )
  {
    *value = detail::step_value( *value, *min, *max, applied_step, true );
    changed = true;
  }
  ImGui::PopItemFlag();
  ImGui::PopID();
  return changed;
}

//! @brief A vertical slider with a + button above and a - button below, each as wide as the slider; no label is rendered.
//! @details Drawn at the current cursor x. Buttons repeat while held and step by kImGuiStepCtrlMultiplier x `step` while
//! Ctrl is held. Ctrl+click on the slider still allows typing an exact value.
//! @param id unique ImGui id for this widget (the text is not rendered)
//! @param size the size of the slider itself, excluding the buttons
//! @return true if the value changed this frame
template <typename T>
bool imgui_step_vslider( const char *id, const ImVec2 &size, T *value, const T *min, const T *max, const char *fmt, T step )
{
  const ImVec2 button_size( size.x, ImGui::GetFrameHeight() );
  const float x = ImGui::GetCursorPosX();
  const T applied_step = ImGui::GetIO().KeyCtrl ? static_cast<T>( step * kImGuiStepCtrlMultiplier ) : step;
  bool changed = false;

  ImGui::PushID( id );
  ImGui::PushItemFlag( ImGuiItemFlags_ButtonRepeat, true );
  if ( ImGui::Button( "+", button_size ) )
  {
    *value = detail::step_value( *value, *min, *max, applied_step, true );
    changed = true;
  }
  ImGui::PopItemFlag();

  ImGui::SetCursorPosX( x );
  changed = ImGui::VSliderScalar( "##vslider", size, detail::imgui_data_type<T>(), value, min, max, fmt ) or changed;

  ImGui::SetCursorPosX( x );
  ImGui::PushItemFlag( ImGuiItemFlags_ButtonRepeat, true );
  if ( ImGui::Button( "-", button_size ) )
  {
    *value = detail::step_value( *value, *min, *max, applied_step, false );
    changed = true;
  }
  ImGui::PopItemFlag();
  ImGui::PopID();
  return changed;
}

//! @brief Width (px) of the empty spacer column between the widget and the label in imgui_labelled_row().
inline constexpr float kImGuiLabelledRowSpacerPx = 5.f;

//! @brief Render a widget and its label as a one-row, three-column table: the widget in a fixed-width first column
//! (the usual item width, so consecutive rows line up), an empty kImGuiLabelledRowSpacerPx spacer column, and the
//! label in a stretched last column, wrapped at its edge.
//! @param id unique ImGui id for the row (e.g. the owning object's address)
//! @param label the text shown in the second column
//! @param draw_widget callable taking the first column's width (float) and drawing the widget without a visible label
template <typename DrawWidget>
void imgui_labelled_row( const void *id, const char *label, DrawWidget &&draw_widget )
{
  const float widget_width = ImGui::CalcItemWidth();
  ImGui::PushID( id );
  if ( ImGui::BeginTable( "##labelled_row", 3 ) )
  {
    ImGui::TableSetupColumn( "##widget", ImGuiTableColumnFlags_WidthFixed, widget_width );
    ImGui::TableSetupColumn( "##spacer", ImGuiTableColumnFlags_WidthFixed, kImGuiLabelledRowSpacerPx );
    ImGui::TableSetupColumn( "##label", ImGuiTableColumnFlags_WidthStretch );
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex( 0 );
    draw_widget( ImGui::GetContentRegionAvail().x );
    ImGui::TableSetColumnIndex( 2 );
    ImGui::AlignTextToFramePadding();
    ImGui::TextWrapped( "%s", label );
    ImGui::EndTable();
  }
  ImGui::PopID();
}

//! @brief Drop-in replacement for ImGui::SliderScalar with -/+ step buttons between the slider and its label.
//! @details The slider and buttons together occupy the usual item width, so it lines up with other widgets.
//! @return true if the value changed this frame
template <typename T>
bool imgui_step_slider( const char *label, T *value, const T *min, const T *max, const char *fmt, T step )
{
  const bool changed = imgui_step_slider_body( label, value, min, max, fmt, step, ImGui::CalcItemWidth() );

  // render the visible part of the label (anything before "##"), as ImGui does for its own widgets
  const char *label_end = std::strstr( label, "##" );
  if ( label_end != label )
  {
    ImGui::SameLine( 0.f, ImGui::GetStyle().ItemInnerSpacing.x );
    ImGui::TextUnformatted( label, label_end );
  }
  return changed;
}

} // namespace Game::Utils

#endif // SRC_UTILS_IMGUISTEPSLIDER_HPP__
