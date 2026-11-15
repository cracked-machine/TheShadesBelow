#ifndef SRC_COMPONENTS_PERSISTENT_MUSICVOLUME_HPP__
#define SRC_COMPONENTS_PERSISTENT_MUSICVOLUME_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Playback volume of the background music.
class MusicVolume : public BasePersistent<float>
{
public:
  //! @brief Construct the music volume setting.
  //! @param value Initial volume level.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  MusicVolume( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "MusicVolume"; }
  //! @brief Returns the display description for this setting (unset).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_MUSICVOLUME_HPP__
