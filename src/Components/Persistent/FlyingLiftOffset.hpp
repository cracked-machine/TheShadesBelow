#ifndef SRC_COMPONENTS_PERSISTENT_FLYINGLIFTOFFSET_HPP__
#define SRC_COMPONENTS_PERSISTENT_FLYINGLIFTOFFSET_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Vertical lift (px) applied to the player sprite while flying. Visual only - the player position stays on the ground.
struct FlyingLiftOffset : public BasePersistent<float>
{
  //! @brief Construct with an initial lift and optional min/max bounds.
  //! @param value initial lift
  //! @param min_value minimum allowed lift
  //! @param max_value maximum allowed lift
  FlyingLiftOffset( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "FlyingLiftOffset"
  virtual std::string class_name() const override { return "FlyingLiftOffset"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_FLYINGLIFTOFFSET_HPP__
