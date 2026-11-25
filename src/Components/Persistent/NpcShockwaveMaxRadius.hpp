#ifndef SRC_COMPONENTS_PERSISTENT_NPCSHOCKWAVEMAXRADIUS_HPP__
#define SRC_COMPONENTS_PERSISTENT_NPCSHOCKWAVEMAXRADIUS_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Maximum radius an NPC shockwave ring expands to before it is removed.
class NpcShockwaveMaxRadius : public BasePersistent<float>
{
public:
  //! @brief Construct the NPC shockwave max radius setting.
  //! @param value Initial maximum radius.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  NpcShockwaveMaxRadius( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "NpcShockwaveMaxRadius"; }
  //! @brief Returns the display description for this setting (unset).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_NPCSHOCKWAVEMAXRADIUS_HPP__
