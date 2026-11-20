#ifndef SRC_COMPONENTS_PERSISTENT_SHOWDAMAGETEXT_HPP__
#define SRC_COMPONENTS_PERSISTENT_SHOWDAMAGETEXT_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Whether a weapon hit on a plant or NPC spawns a floating damage number (Cmp::DamagePopup). See
//! Utils::apply_weapon_hit() and HudOverlaySystem::render_damage_popups().
class ShowDamageText : public BasePersistent<bool>
{
public:
  //! @brief Construct the setting.
  //! @param value true to show damage numbers.
  ShowDamageText( bool value = false )
      : BasePersistent<bool>( value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "ShowDamageText"; }
  //! @brief Returns the display description for this setting (unset; see persist.json).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_SHOWDAMAGETEXT_HPP__
