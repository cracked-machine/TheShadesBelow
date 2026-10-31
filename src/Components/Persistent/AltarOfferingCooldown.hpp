#ifndef SRC_COMPONENTS_PERSISTENT_ALTAROFFERINGCOOLDOWN_HPP__
#define SRC_COMPONENTS_PERSISTENT_ALTAROFFERINGCOOLDOWN_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Altar sacrifice cooldown (seconds) after an offering, before the next one is accepted. A
//! Cmp::Npc::SummonOffering uses Cmp::Persist::AltarSummonOfferingCooldown instead.
struct AltarOfferingCooldown : public BasePersistent<float>
{
  //! @brief Construct with an initial cooldown and optional min/max bounds.
  //! @param value initial cooldown, in seconds
  //! @param min_value minimum allowed cooldown
  //! @param max_value maximum allowed cooldown
  AltarOfferingCooldown( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "AltarOfferingCooldown"
  virtual std::string class_name() const override { return "AltarOfferingCooldown"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_ALTAROFFERINGCOOLDOWN_HPP__
