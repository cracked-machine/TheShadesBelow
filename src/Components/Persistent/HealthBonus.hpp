#ifndef SRC_COMPONENTS_PERSISTENT_HEALTHBONUS_HPP__
#define SRC_COMPONENTS_PERSISTENT_HEALTHBONUS_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Bonus applied to the player's maximum health, e.g. from loot pickups.
class HealthBonus : public BasePersistent<uint8_t>
{
public:
  //! @brief Construct the health bonus setting.
  //! @param value Initial health bonus amount.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  HealthBonus( uint8_t value = 0, uint8_t min_value = 0, uint8_t max_value = 0 )
      : BasePersistent<uint8_t>( value, min_value, max_value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "HealthBonus"; }
  //! @brief Returns the display description for this setting (unset).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_HEALTHBONUS_HPP__
