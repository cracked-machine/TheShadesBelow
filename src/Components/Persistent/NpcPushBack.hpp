#ifndef SRC_COMPONENTS_PERSISTENT_NPCPUSHBACK_HPP__
#define SRC_COMPONENTS_PERSISTENT_NPCPUSHBACK_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Distance/force an NPC is pushed back by on collision.
class NpcPushBack : public BasePersistent<float>
{
public:
  //! @brief Construct the NPC pushback setting.
  //! @param value Initial pushback amount.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  NpcPushBack( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "NpcPushBack"; }
  //! @brief Returns the display description for this setting (unset).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_NPCPUSHBACK_HPP__
