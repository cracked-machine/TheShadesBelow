#ifndef SRC_COMPONENTS_PERSISTENT_HAZARDBALANCEOVERLAP_HPP__
#define SRC_COMPONENTS_PERSISTENT_HAZARDBALANCEOVERLAP_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief How far (fraction of a grid cell) the player can step onto a lethal hazard cell (sinkhole/lava pit)
//!        before they are stopped and start balancing on its edge. See Cmp::Player::HazardBalance and
//!        Utils::Collision::lethal_hazard_probe(). Capped at 0.5: the probe is centred on the player.
struct HazardBalanceOverlap : BasePersistent<float>
{
  //! @brief Construct the hazard balance overlap setting.
  //! @param value Initial overlap, as a fraction of a grid cell.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  HazardBalanceOverlap( float value = 0, float min_value = 0, float max_value = 0 )
      : BasePersistent( value, min_value, max_value )
  {
  }

  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "HazardBalanceOverlap"; }
  //! @brief Returns the display description for this setting (unset).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_HAZARDBALANCEOVERLAP_HPP__
