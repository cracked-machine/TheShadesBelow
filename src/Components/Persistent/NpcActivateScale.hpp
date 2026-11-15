#ifndef SRC_COMPONENTS_PERSISTENT_NPCACTIVATESCALE_HPP__
#define SRC_COMPONENTS_PERSISTENT_NPCACTIVATESCALE_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Scale multiplier applied to an NPC container's bounds when testing whether the player is
//!        close enough to activate (spawn) the NPC inside it.
class NpcActivateScale : public BasePersistent<float>
{
public:
  //! @brief Construct the NPC activation scale setting.
  //! @param value Initial scale multiplier.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  NpcActivateScale( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "NpcActivateScale"; }
  //! @brief Returns the display description for this setting (unset).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_NPCACTIVATESCALE_HPP__
