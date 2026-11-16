#ifndef SRC_COMPONENTS_PERSISTENT_HAZARDBALANCETIMEOUT_HPP__
#define SRC_COMPONENTS_PERSISTENT_HAZARDBALANCETIMEOUT_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief How long (seconds) the player balances on the edge of a lethal hazard cell (sinkhole/lava pit)
//!        before falling in. See Cmp::Player::HazardBalance.
struct HazardBalanceTimeout : BasePersistent<float>
{
  //! @brief Construct the hazard balance timeout setting.
  //! @param value Initial balance timeout, in seconds.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  HazardBalanceTimeout( float value = 0, float min_value = 0, float max_value = 0 )
      : BasePersistent( value, min_value, max_value )
  {
  }

  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "HazardBalanceTimeout"; }
  //! @brief Returns the display description for this setting (unset).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_HAZARDBALANCETIMEOUT_HPP__
