#ifndef SRC_COMPONENTS_PERSISTENT_TACHYCARDIAHEALTHDMGPERSEC_HPP__
#define SRC_COMPONENTS_PERSISTENT_TACHYCARDIAHEALTHDMGPERSEC_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Health lost by the player each second while tachycardia toxicity is at maximum.
class TachycardiaHealthDmgPerSec : public BasePersistent<uint8_t>
{
public:
  //! @brief Construct the tachycardia health dmg per sec setting.
  //! @param value Initial health damage per second.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  TachycardiaHealthDmgPerSec( uint8_t value = 0, uint8_t min_value = 0, uint8_t max_value = 0 )
      : BasePersistent<uint8_t>( value, min_value, max_value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  virtual std::string class_name() const override { return "TachycardiaHealthDmgPerSec"; }
  //! @brief Returns the display description for this setting (unset).
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_TACHYCARDIAHEALTHDMGPERSEC_HPP__
