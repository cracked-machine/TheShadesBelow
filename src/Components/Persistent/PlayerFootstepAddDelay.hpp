#ifndef SRC_COMPONENTS_PERSISTENT_PLAYERFOOTSTEPADDDELAY_HPP__
#define SRC_COMPONENTS_PERSISTENT_PLAYERFOOTSTEPADDDELAY_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Persistent setting for the minimum delay (in seconds) between spawning new player
//! footstep effects.
class PlayerFootstepAddDelay : public BasePersistent<float>
{
public:
  //! @brief Construct a new PlayerFootstepAddDelay object.
  //! @param value Initial delay value.
  //! @param min_value Minimum allowed delay value.
  //! @param max_value Maximum allowed delay value.
  PlayerFootstepAddDelay( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Returns the class name used for identification and JSON serialization.
  //! @return std::string The class name.
  [[nodiscard]] std::string class_name() const override { return "PlayerFootstepAddDelay"; }
  //! @brief Returns the display detail string for this setting (currently unused/empty).
  //! @return std::string The detail string.
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_PLAYERFOOTSTEPADDDELAY_HPP__
