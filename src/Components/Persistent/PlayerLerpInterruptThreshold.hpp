#ifndef SRC_COMPONENTS_PERSISTENT_PLAYERLERPINTERRUPTTHRESHOLD_HPP__
#define SRC_COMPONENTS_PERSISTENT_PLAYERLERPINTERRUPTTHRESHOLD_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Controls how far into a lerp the player can still change direction
//! 0.0 = no interruption allowed (must complete lerp)
//! 0.25 = can interrupt in first 25% of lerp
//! 0.5 = can interrupt in first 50% of lerp
//! 1.0 = can always interrupt (very responsive, but may feel jittery)
struct PlayerLerpInterruptThreshold : BasePersistent<float>
{
  //! @brief Construct a new PlayerLerpInterruptThreshold object.
  //! @param value Initial threshold value.
  //! @param min Minimum allowed threshold value.
  //! @param max Maximum allowed threshold value.
  PlayerLerpInterruptThreshold( float value = 0.f, float min = 0.f, float max = 0.f )
      : BasePersistent( value, min, max )
  {
  }
  //! @brief Returns the class name used for identification and JSON serialization.
  //! @return std::string The class name.
  [[nodiscard]] std::string class_name() const override { return "PlayerLerpInterruptThreshold"; }
  //! @brief Returns the display detail string for this setting (currently unused/empty).
  //! @return std::string The detail string.
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_PLAYERLERPINTERRUPTTHRESHOLD_HPP__
