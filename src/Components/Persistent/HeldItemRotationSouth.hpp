#ifndef SRC_COMPONENTS_PERSISTENT_HELDITEMROTATIONSOUTH_HPP__
#define SRC_COMPONENTS_PERSISTENT_HELDITEMROTATIONSOUTH_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Rotation (degrees, clockwise) of the held inventory item while the player faces south, about the item's centre.
//! West uses Cmp::Persist::HeldItemRotationWest; east and north use Cmp::Persist::HeldItemRotation.
struct HeldItemRotationSouth : public BasePersistent<float>
{
  //! @brief Construct with an initial value and optional min/max bounds.
  //! @param value initial value
  //! @param min_value minimum allowed value
  //! @param max_value maximum allowed value
  HeldItemRotationSouth( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "HeldItemRotationSouth"
  virtual std::string class_name() const override { return "HeldItemRotationSouth"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_HELDITEMROTATIONSOUTH_HPP__
