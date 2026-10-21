#include <Components/Persistent/BasePersistent.hpp>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Game::Cmp::Persist
{

void log_msg( const std::string &msg, const std::source_location &loc )
{
  if ( spdlog::default_logger()->should_log( spdlog::level::debug ) )
  {
    spdlog::log( spdlog::source_loc{ loc.file_name(), static_cast<int>( loc.line() ), loc.function_name() }, spdlog::level::debug, "{}", msg );
  }
}

template <typename T>
BasePersistent<T>::BasePersistent( T value_in, T min_value_in, T max_value_in )
    : value( value_in ),
      min_value( min_value_in ),
      max_value( max_value_in )
{
}

template <typename T>
nlohmann::json BasePersistent<T>::serialize() const
{
  nlohmann::json json_data;
  if constexpr ( std::is_same_v<T, bool> )
  {
    // booleans have no range or format, only a value and a label
    json_data["type"] = "bool";
    json_data["value"] = value;
    json_data["detail"] = detail;
    return json_data;
  }
  if constexpr ( std::is_floating_point_v<T> ) { json_data["type"] = "float"; }
  else if constexpr ( std::is_integral_v<T> ) { json_data["type"] = "int"; }
  json_data["value"] = value;
  json_data["min_value"] = min_value;
  json_data["max_value"] = max_value;
  json_data["detail"] = detail;
  json_data["format"] = format;
  return json_data;
}

template <typename T>
void BasePersistent<T>::deserialize( const nlohmann::json &json_data )
{
  if constexpr ( std::is_same_v<T, bool> )
  {
    // booleans have no range or format, only a value and a label
    if ( json_data.contains( "value" ) && json_data["value"].is_boolean() ) value = json_data["value"].get<bool>();
    if ( json_data.contains( "detail" ) && json_data["detail"].is_string() ) { detail = json_data["detail"].get<std::string>(); }
    if ( detail.empty() ) { log_msg( "InvalidJSONField: 'details' for " + class_name() ); }
    return;
  }

  if ( json_data.contains( "value" ) && json_data["value"].is_number() ) value = json_data["value"].get<T>();

  if ( json_data.contains( "min_value" ) && json_data["min_value"].is_number() ) { min_value = json_data["min_value"].get<T>(); }

  if ( json_data.contains( "max_value" ) && json_data["max_value"].is_number() ) { max_value = json_data["max_value"].get<T>(); }

  if ( json_data.contains( "detail" ) )
  {
    if ( json_data["detail"].is_string() ) { detail = json_data["detail"].get<std::string>(); }
  }
  else { log_msg( "detail field NOT found in json" ); }

  if ( json_data.contains( "format" ) )
  {
    if ( json_data["format"].is_string() ) { format = json_data["format"].get<std::string>(); }
  }
  else { log_msg( "format field NOT found in json" ); }

  if ( format.empty() ) { log_msg( "InvalidJSONField: 'format' for " + class_name() ); }
  if ( detail.empty() ) { log_msg( "InvalidJSONField: 'details' for " + class_name() ); }
}

template <typename T>
void BasePersistent<T>::render_widget()
{
  ImGui::PushID( this );

  const char *label = detail.empty() ? "<Invalid Label>" : detail.c_str();

  if constexpr ( std::is_same_v<T, bool> ) { ImGui::Checkbox( label, get_value_ptr() ); }
  else if constexpr ( std::is_same_v<T, float> )
  {
    const char *fmt = format.empty() ? "%.3f" : format.c_str();
    ImGui::SliderScalar( label, ImGuiDataType_Float, get_value_ptr(), get_min_value_ptr(), get_max_value_ptr(), fmt );
  }
  else if constexpr ( std::is_same_v<T, uint8_t> )
  {
    const char *fmt = format.empty() ? "%d" : format.c_str();
    ImGui::SliderScalar( label, ImGuiDataType_U8, get_value_ptr(), get_min_value_ptr(), get_max_value_ptr(), fmt );
  }
  else if constexpr ( std::is_same_v<T, uint16_t> )
  {
    const char *fmt = format.empty() ? "%d" : format.c_str();
    ImGui::SliderScalar( label, ImGuiDataType_U16, get_value_ptr(), get_min_value_ptr(), get_max_value_ptr(), fmt );
  }
  else if constexpr ( std::is_same_v<T, uint32_t> )
  {
    const char *fmt = format.empty() ? "%u" : format.c_str();
    ImGui::SliderScalar( label, ImGuiDataType_U32, get_value_ptr(), get_min_value_ptr(), get_max_value_ptr(), fmt );
  }
  else if constexpr ( std::is_same_v<T, uint64_t> )
  {
    const char *fmt = format.empty() ? "%llu" : format.c_str();
    // ImGui requires max_value <= IM_U64_MAX / 2 for sliders
    constexpr uint64_t imgui_max_limit = std::numeric_limits<uint64_t>::max() / 2;
    uint64_t clamped_max = std::min( max_value, imgui_max_limit );
    ImGui::SliderScalar( label, ImGuiDataType_U64, get_value_ptr(), get_min_value_ptr(), &clamped_max, fmt );
  }
  else if constexpr ( std::is_same_v<T, int32_t> || std::is_same_v<T, int> )
  {
    const char *fmt = format.empty() ? "%d" : format.c_str();
    ImGui::SliderScalar( label, ImGuiDataType_S32, get_value_ptr(), get_min_value_ptr(), get_max_value_ptr(), fmt );
  }
  else if constexpr ( std::is_same_v<T, int64_t> )
  {
    const char *fmt = format.empty() ? "%lld" : format.c_str();
    // ImGui requires max_value <= IM_S64_MAX / 2 for sliders
    constexpr int64_t imgui_max_limit = std::numeric_limits<int64_t>::max() / 2;
    int64_t clamped_max = std::min( max_value, imgui_max_limit );
    ImGui::SliderScalar( label, ImGuiDataType_S64, get_value_ptr(), get_min_value_ptr(), &clamped_max, fmt );
  }
  else
  {
    // Fallback for unsupported types - just show the value as text
    ImGui::Text( "%s: (unsupported type)", label );
  }

  ImGui::PopID();
}

// Explicit instantiations
template class BasePersistent<bool>;
template class BasePersistent<float>;
template class BasePersistent<int>;
template class BasePersistent<int64_t>;
template class BasePersistent<uint8_t>;
template class BasePersistent<uint16_t>;
template class BasePersistent<uint32_t>;
template class BasePersistent<uint64_t>;

} // namespace Game::Cmp::Persist