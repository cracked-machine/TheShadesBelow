#ifndef SRC_COMPONENTS_PERSISTENT_SHOWHEALTHBAR_HPP__
#define SRC_COMPONENTS_PERSISTENT_SHOWHEALTHBAR_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Whether an ImGui health bar is drawn along the top of each damaged (Cmp::Health below 100) on-screen plant
//! and NPC. See HudOverlaySystem::render_world_health_bars().
class ShowHealthBar : public BasePersistent<bool>
{
public:
  //! @brief Construct the setting.
  //! @param value true to show health bars.
  ShowHealthBar( bool value = false )
      : BasePersistent<bool>( value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "ShowHealthBar"; }
  //! @brief Returns the display description for this setting (unset; see persist.json).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_SHOWHEALTHBAR_HPP__
