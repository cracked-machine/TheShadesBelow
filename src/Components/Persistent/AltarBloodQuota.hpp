#ifndef SRC_COMPONENTS_PERSISTENT_ALTARBLOODQUOTA_HPP__
#define SRC_COMPONENTS_PERSISTENT_ALTARBLOODQUOTA_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Blood a graveyard altar must be offered (poured from blood vials, see Cmp::WorldItem::get_blood_heal()) before it
//! activates, lighting its centre column and sending a crow to the key grave. See Sys::AltarSystem and Cmp::Altar::BloodCount.
class AltarBloodQuota : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial quota and optional min/max bounds.
  //! @param value initial blood quota
  //! @param min_value minimum allowed blood quota
  //! @param max_value maximum allowed blood quota
  AltarBloodQuota( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "AltarBloodQuota"
  [[nodiscard]] std::string class_name() const override { return "AltarBloodQuota"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_ALTARBLOODQUOTA_HPP__
