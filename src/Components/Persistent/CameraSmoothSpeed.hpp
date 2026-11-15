#ifndef SRC_COMPONENTS_PERSISTENT_CAMERASMOOTHSPEED_HPP__
#define SRC_COMPONENTS_PERSISTENT_CAMERASMOOTHSPEED_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Controls the smoothing effect on the camera pan. Lower: smoother, Higher: snappier
struct CameraSmoothSpeed : BasePersistent<float>
{
  //! @brief Construct with an initial smoothing speed and optional min/max bounds.
  //! @param value initial smoothing speed
  //! @param min_value minimum allowed smoothing speed
  //! @param max_value maximum allowed smoothing speed
  CameraSmoothSpeed( float value = 0, float min_value = 0, float max_value = 0 )
      : BasePersistent( value, min_value, max_value )
  {
  }

  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "CameraSmoothSpeed"
  [[nodiscard]] std::string class_name() const override { return "CameraSmoothSpeed"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_CAMERASMOOTHSPEED_HPP__
