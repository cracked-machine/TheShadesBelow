#ifndef SRC_COMPONENTS_PERSISTENT_PLAYERANIMSTRIDELENGTH_HPP__
#define SRC_COMPONENTS_PERSISTENT_PLAYERANIMSTRIDELENGTH_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Persistent setting for the distance (in pixels) the player must travel between
//! walk-cycle animation frame changes. Since the trigger is distance-based, this scales the
//! animation cadence with the player's current speed (including any active Cmp::Player::SpeedPenalty).
class PlayerAnimStrideLength : public BasePersistent<float>
{
public:
  //! @brief Construct a new PlayerAnimStrideLength object.
  //! @param value Initial stride length value.
  //! @param min_value Minimum allowed stride length value.
  //! @param max_value Maximum allowed stride length value.
  PlayerAnimStrideLength( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Returns the class name used for identification and JSON serialization.
  //! @return std::string The class name.
  virtual std::string class_name() const override { return "PlayerAnimStrideLength"; }
  //! @brief Returns the display detail string for this setting (currently unused/empty).
  //! @return std::string The detail string.
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_PLAYERANIMSTRIDELENGTH_HPP__
