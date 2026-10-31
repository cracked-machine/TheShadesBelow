#ifndef SRC_COMPONENTS_PERSISTENT_BURRTHROWCOOLDOWN_HPP__
#define SRC_COMPONENTS_PERSISTENT_BURRTHROWCOOLDOWN_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Seconds after a burr enters the inventory, or one is thrown, before another burr can be thrown.
struct BurrThrowCooldown : public BasePersistent<float>
{
  //! @brief Construct with an initial cooldown and optional min/max bounds.
  //! @param value initial cooldown, in seconds
  //! @param min_value minimum allowed cooldown
  //! @param max_value maximum allowed cooldown
  BurrThrowCooldown( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "BurrThrowCooldown"
  virtual std::string class_name() const override { return "BurrThrowCooldown"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_BURRTHROWCOOLDOWN_HPP__
