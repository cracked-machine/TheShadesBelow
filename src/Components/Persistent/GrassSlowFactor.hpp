#ifndef SRC_COMPONENTS_PERSISTENT_GRASSSLOWFACTOR_HPP__
#define SRC_COMPONENTS_PERSISTENT_GRASSSLOWFACTOR_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Multiplier applied to the movement speed of the player and ground NPCs while they walk through grass.
//! At 0 grass blocks the player instead (and slows nothing), see Utils::is_blocked_by_grass().
class GrassSlowFactor : public BasePersistent<float>
{
public:
  //! @brief Construct a new GrassSlowFactor object.
  //! @param value Initial modifier value.
  //! @param min_value Minimum allowed modifier value.
  //! @param max_value Maximum allowed modifier value.
  GrassSlowFactor( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Returns the class name used for identification and JSON serialization.
  //! @return std::string The class name.
  [[nodiscard]] std::string class_name() const override { return "GrassSlowFactor"; }
  //! @brief Returns the display detail string for this setting (currently unused/empty).
  //! @return std::string The detail string.
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_GRASSSLOWFACTOR_HPP__
