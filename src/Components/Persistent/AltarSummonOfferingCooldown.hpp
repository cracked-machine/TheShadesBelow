#ifndef SRC_COMPONENTS_PERSISTENT_ALTARSUMMONOFFERINGCOOLDOWN_HPP__
#define SRC_COMPONENTS_PERSISTENT_ALTARSUMMONOFFERINGCOOLDOWN_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Altar sacrifice cooldown (seconds) when offering a Cmp::Npc::SummonOffering, e.g. Azazel's jewelry, so they can be
//! sacrificed in quick succession. Other offerings use Cmp::Persist::AltarOfferingCooldown.
struct AltarSummonOfferingCooldown : public BasePersistent<float>
{
  //! @brief Construct with an initial cooldown and optional min/max bounds.
  //! @param value initial cooldown, in seconds
  //! @param min_value minimum allowed cooldown
  //! @param max_value maximum allowed cooldown
  AltarSummonOfferingCooldown( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "AltarSummonOfferingCooldown"
  virtual std::string class_name() const override { return "AltarSummonOfferingCooldown"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_ALTARSUMMONOFFERINGCOOLDOWN_HPP__
