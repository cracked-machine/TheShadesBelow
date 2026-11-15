#ifndef SRC_COMPONENTS_PERSISTENT_PLAYERMOVEMENTSPEED_HPP__
#define SRC_COMPONENTS_PERSISTENT_PLAYERMOVEMENTSPEED_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Persistent setting for the player character's base movement speed.
class PlayerMovementSpeed : public BasePersistent<float>
{
public:
  //! @brief Construct a new PlayerMovementSpeed object.
  //! @param value Initial movement speed value.
  //! @param min_value Minimum allowed movement speed value.
  //! @param max_value Maximum allowed movement speed value.
  PlayerMovementSpeed( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Returns the class name used for identification and JSON serialization.
  //! @return std::string The class name.
  [[nodiscard]] std::string class_name() const override { return "PlayerMovementSpeed"; }
  //! @brief Returns the display detail string for this setting.
  //! @return std::string The detail string.
  [[nodiscard]] const std::string get_detail() const override { return detail; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_PLAYERMOVEMENTSPEED_HPP__
