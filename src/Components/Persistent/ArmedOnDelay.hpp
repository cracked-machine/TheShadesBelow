#ifndef SRC_COMPONENTS_PERSISTENT_ARMEDONDELAY_HPP__
#define SRC_COMPONENTS_PERSISTENT_ARMEDONDELAY_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Delay between each concentric obstacle in a bomb pattern being armed.
class ArmedOnDelay : public BasePersistent<float>
{
public:
  //! @brief Construct with an initial delay and optional min/max bounds.
  //! @param value initial delay
  //! @param min_value minimum allowed delay
  //! @param max_value maximum allowed delay
  ArmedOnDelay( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }

  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "ArmedOnDelay"
  [[nodiscard]] std::string class_name() const override { return "ArmedOnDelay"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_ARMEDONDELAY_HPP__
