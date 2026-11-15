#ifndef SRC_COMPONENTS_PERSISTENT_DIGGINGCOOLDOWNTHRESHOLD_HPP__
#define SRC_COMPONENTS_PERSISTENT_DIGGINGCOOLDOWNTHRESHOLD_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Minimum time, in seconds, the player must wait between successive digging actions.
struct DiggingCooldownThreshold : public BasePersistent<float>
{
  //! @brief Construct with an initial cooldown value and optional min/max bounds.
  //! @param value initial cooldown value, in seconds
  //! @param min_value minimum allowed cooldown value
  //! @param max_value maximum allowed cooldown value
  DiggingCooldownThreshold( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "DiggingCooldownThreshold"
  [[nodiscard]] std::string class_name() const override { return "DiggingCooldownThreshold"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_DIGGINGCOOLDOWNTHRESHOLD_HPP__
