#ifndef SRC_COMPONENTS_PERSISTENT_HELDITEMROTATIONWEST_HPP__
#define SRC_COMPONENTS_PERSISTENT_HELDITEMROTATIONWEST_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Rotation (degrees, clockwise) of the held inventory item while the player faces west, about the item's centre.
//! The item is mirrored horizontally when facing west, so 360 - HeldItemRotation gives the mirror image of the east pose.
//! South uses Cmp::Persist::HeldItemRotationSouth; east and north use Cmp::Persist::HeldItemRotation.
struct HeldItemRotationWest : public BasePersistent<float>
{
  //! @brief Construct with an initial value and optional min/max bounds.
  //! @param value initial value
  //! @param min_value minimum allowed value
  //! @param max_value maximum allowed value
  HeldItemRotationWest( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "HeldItemRotationWest"
  [[nodiscard]] std::string class_name() const override { return "HeldItemRotationWest"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_HELDITEMROTATIONWEST_HPP__
