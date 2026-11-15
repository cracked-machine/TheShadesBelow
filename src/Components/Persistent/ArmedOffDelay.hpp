#ifndef SRC_COMPONENTS_PERSISTENT_ARMEDOFFDELAY_HPP__
#define SRC_COMPONENTS_PERSISTENT_ARMEDOFFDELAY_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Delay between each concentric obstacle in a bomb pattern being disarmed (exploding).
class ArmedOffDelay : public BasePersistent<float>
{
public:
  //! @brief Construct with an initial delay and optional min/max bounds.
  //! @param delay initial delay
  //! @param min_value minimum allowed delay
  //! @param max_value maximum allowed delay
  ArmedOffDelay( float delay = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( delay, min_value, max_value )
  {
  }

  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "ArmedOffDelay"
  [[nodiscard]] std::string class_name() const override { return "ArmedOffDelay"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_ARMEDOFFDELAY_HPP__
