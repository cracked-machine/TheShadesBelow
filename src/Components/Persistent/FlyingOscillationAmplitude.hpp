#ifndef SRC_COMPONENTS_PERSISTENT_FLYINGOSCILLATIONAMPLITUDE_HPP__
#define SRC_COMPONENTS_PERSISTENT_FLYINGOSCILLATIONAMPLITUDE_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Max amplitude (px) of the random vertical oscillation applied to the player sprite while flying.
struct FlyingOscillationAmplitude : public BasePersistent<float>
{
  //! @brief Construct with an initial amplitude and optional min/max bounds.
  //! @param value initial amplitude
  //! @param min_value minimum allowed amplitude
  //! @param max_value maximum allowed amplitude
  FlyingOscillationAmplitude( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "FlyingOscillationAmplitude"
  virtual std::string class_name() const override { return "FlyingOscillationAmplitude"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_FLYINGOSCILLATIONAMPLITUDE_HPP__
