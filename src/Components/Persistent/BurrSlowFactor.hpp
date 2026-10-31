#ifndef SRC_COMPONENTS_PERSISTENT_BURRSLOWFACTOR_HPP__
#define SRC_COMPONENTS_PERSISTENT_BURRSLOWFACTOR_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Movement speed multiplier for each burr; burrs compound, e.g. 0.75 with two burrs gives 0.5625.
struct BurrSlowFactor : public BasePersistent<float>
{
  //! @brief Construct with an initial slow factor and optional min/max bounds.
  //! @param value initial slow factor
  //! @param min_value minimum allowed slow factor
  //! @param max_value maximum allowed slow factor
  BurrSlowFactor( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "BurrSlowFactor"
  virtual std::string class_name() const override { return "BurrSlowFactor"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_BURRSLOWFACTOR_HPP__
