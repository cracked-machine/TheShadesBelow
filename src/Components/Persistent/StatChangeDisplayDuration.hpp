#ifndef SRC_COMPONENTS_PERSISTENT_STATCHANGEDISPLAYDURATION_HPP__
#define SRC_COMPONENTS_PERSISTENT_STATCHANGEDISPLAYDURATION_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief How long (in seconds) a "<stat>_delta_lbl" UI label shows the change in its player stat or toxidrome
//! before it is cleared.
class StatChangeDisplayDuration : public BasePersistent<float>
{
public:
  //! @brief Construct a new StatChangeDisplayDuration object.
  //! @param value Initial duration, in seconds.
  //! @param min_value Minimum allowed duration.
  //! @param max_value Maximum allowed duration.
  StatChangeDisplayDuration( float value = 1.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }

  //! @brief Returns the class name used for identification and JSON serialization.
  //! @return std::string The class name.
  [[nodiscard]] std::string class_name() const override { return "StatChangeDisplayDuration"; }
  //! @brief Returns the display detail string for this setting (currently unused/empty).
  //! @return std::string The detail string.
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_STATCHANGEDISPLAYDURATION_HPP__
