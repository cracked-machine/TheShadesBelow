#ifndef SRC_COMPONENTS_PERSISTENT_HELDITEMSCALE_HPP__
#define SRC_COMPONENTS_PERSISTENT_HELDITEMSCALE_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Scale of the held inventory item drawn on the player, for all facing directions. Multiplied with the
//! player's Cmp::AbsoluteScale; the item scales about its centre.
struct HeldItemScale : public BasePersistent<float>
{
  //! @brief Construct with an initial scale and optional min/max bounds.
  //! @param value initial scale
  //! @param min_value minimum allowed scale
  //! @param max_value maximum allowed scale
  HeldItemScale( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "HeldItemScale"
  virtual std::string class_name() const override { return "HeldItemScale"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_HELDITEMSCALE_HPP__
