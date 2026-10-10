#ifndef SRC_COMPONENTS_PERSISTENT_NPCDEATHANIMFRAMERATE_HPP__
#define SRC_COMPONENTS_PERSISTENT_NPCDEATHANIMFRAMERATE_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Playback rate of the NPC death animation.
class NpcDeathAnimFramerate : public BasePersistent<float>
{
public:
  //! @brief Construct the NPC death animation framerate setting.
  //! @note The unit is frames per second. 0 = not animated.
  //! @param framerate Initial framerate, in frames per second.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  NpcDeathAnimFramerate( float framerate = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( framerate, min_value, max_value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  virtual std::string class_name() const override { return "NpcDeathAnimFramerate"; }
  //! @brief Returns the display description for this setting (unset).
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_NPCDEATHANIMFRAMERATE_HPP__
