#ifndef SRC_COMPONENTS_PERSISTENT_STOWEDTORCHLIT_HPP__
#define SRC_COMPONENTS_PERSISTENT_STOWEDTORCHLIT_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Whether a torch in an inventory slot that is not selected stays lit, i.e. still lights the player
//! (Shaders::NightStaticShader) and shows its flame particle over its HUD slot.
//! @note Only light and the HUD flame are affected; torch actions and carry effects still need the torch selected.
//! See Utils::Player::is_torch_slot_lit().
class StowedTorchLit : public BasePersistent<bool>
{
public:
  //! @brief Construct the stowed torch lit setting.
  //! @param value true to keep stowed torches lit.
  StowedTorchLit( bool value = false )
      : BasePersistent<bool>( value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "StowedTorchLit"; }
  //! @brief Returns the display description for this setting (unset; see persist.json).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_STOWEDTORCHLIT_HPP__
