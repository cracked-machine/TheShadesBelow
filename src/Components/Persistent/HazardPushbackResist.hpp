#ifndef SRC_COMPONENTS_PERSISTENT_HAZARDPUSHBACKRESIST_HPP__
#define SRC_COMPONENTS_PERSISTENT_HAZARDPUSHBACKRESIST_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief How long (seconds) the player must keep pushing into a lethal hazard cell before
//!        Cmp::Hazard::CollisionResist yields and lets them step/fall in.
struct HazardPushbackResist : BasePersistent<float>
{
  //! @brief Construct the hazard pushback resistance setting.
  //! @param value Initial resistance time, in seconds.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  HazardPushbackResist( float value = 0, float min_value = 0, float max_value = 0 )
      : BasePersistent( value, min_value, max_value )
  {
  }

  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "HazardPushbackResist"; }
  //! @brief Returns the display description for this setting (unset).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_HAZARDPUSHBACKRESIST_HPP__
