#ifndef SRC_COMPONENTS_PERSISTENT_MELEEHITCOOLDOWN_HPP__
#define SRC_COMPONENTS_PERSISTENT_MELEEHITCOOLDOWN_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Minimum time between shovel melee hits on the same NPC, in seconds. Melee is checked every frame the attack
//! button is held, so without it one press would hit an NPC many times. See Sys::ActionSystem::check_player_melee_npc_kill().
struct MeleeHitCooldown : public BasePersistent<float>
{
  //! @brief Construct with an initial cooldown and optional min/max bounds.
  //! @param value initial cooldown in seconds
  //! @param min_value minimum allowed cooldown
  //! @param max_value maximum allowed cooldown
  MeleeHitCooldown( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "MeleeHitCooldown"
  [[nodiscard]] std::string class_name() const override { return "MeleeHitCooldown"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_MELEEHITCOOLDOWN_HPP__
