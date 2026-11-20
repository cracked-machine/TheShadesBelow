#ifndef SRC_COMPONENTS_PERSISTENT_ALTARBLOODOFFERINGHEALTH_HPP__
#define SRC_COMPONENTS_PERSISTENT_ALTARBLOODOFFERINGHEALTH_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Health the player bleeds into a graveyard altar each time they make an offering at it with nothing else to
//! offer, adding the same amount to its Cmp::Altar::BloodCount. See Sys::AltarSystem and Cmp::Persist::AltarBloodQuota.
class AltarBloodOfferingHealth : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial value and optional min/max bounds.
  //! @param value initial health taken per offering
  //! @param min_value minimum allowed health taken per offering
  //! @param max_value maximum allowed health taken per offering
  AltarBloodOfferingHealth( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "AltarBloodOfferingHealth"
  [[nodiscard]] std::string class_name() const override { return "AltarBloodOfferingHealth"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_ALTARBLOODOFFERINGHEALTH_HPP__
