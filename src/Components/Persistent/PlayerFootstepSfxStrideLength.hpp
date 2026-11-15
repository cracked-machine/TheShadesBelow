#ifndef SRC_COMPONENTS_PERSISTENT_PLAYERFOOTSTEPSFXSTRIDELENGTH_HPP__
#define SRC_COMPONENTS_PERSISTENT_PLAYERFOOTSTEPSFXSTRIDELENGTH_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Persistent setting for the distance (in pixels) the player must travel between
//! footstep sound effects. Since the trigger is distance-based, this scales the sfx cadence
//! with the player's current speed (including any active Cmp::Player::SpeedPenalty).
class PlayerFootstepSfxStrideLength : public BasePersistent<float>
{
public:
  //! @brief Construct a new PlayerFootstepSfxStrideLength object.
  //! @param value Initial stride length value.
  //! @param min_value Minimum allowed stride length value.
  //! @param max_value Maximum allowed stride length value.
  PlayerFootstepSfxStrideLength( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Returns the class name used for identification and JSON serialization.
  //! @return std::string The class name.
  [[nodiscard]] std::string class_name() const override { return "PlayerFootstepSfxStrideLength"; }
  //! @brief Returns the display detail string for this setting (currently unused/empty).
  //! @return std::string The detail string.
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_PLAYERFOOTSTEPSFXSTRIDELENGTH_HPP__
