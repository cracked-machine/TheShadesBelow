#ifndef SRC_COMPONENTS_PERSISTENT_NPCSHOCKWAVERESOLUTION_HPP__
#define SRC_COMPONENTS_PERSISTENT_NPCSHOCKWAVERESOLUTION_HPP__

#include <Components/Npc/Shockwave.hpp>
#include <Components/Persistent/BasePersistent.hpp>
#include <cstdint>

namespace Game::Cmp::Persist
{

//! @brief Number of segments used to approximate the circular NPC shockwave ring.
class NpcShockwaveResolution : public BasePersistent<uint16_t>
{
public:
  //! @brief Construct the NPC shockwave resolution setting.
  //! @param value Initial number of circle segments.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  NpcShockwaveResolution( uint16_t value = 0, uint16_t min_value = 0, uint16_t max_value = 0 )
      : BasePersistent<uint16_t>( value, min_value, max_value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "NpcShockwaveResolution"; }
  //! @brief Returns the display description for this setting (unset).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_NPCSHOCKWAVERESOLUTION_HPP__
