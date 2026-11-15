#ifndef SRC_COMPONENTS_PERSISTENT_ALTARACTIVATIONANIMACOST_HPP__
#define SRC_COMPONENTS_PERSISTENT_ALTARACTIVATIONANIMACOST_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Anima the player pays to activate a graveyard altar once all its artefacts are offered, lighting its centre
//! column and starting the grave trail. See Sys::AltarSystem.
class AltarActivationAnimaCost : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial cost and optional min/max bounds.
  //! @param value initial anima cost
  //! @param min_value minimum allowed anima cost
  //! @param max_value maximum allowed anima cost
  AltarActivationAnimaCost( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "AltarActivationAnimaCost"
  [[nodiscard]] std::string class_name() const override { return "AltarActivationAnimaCost"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_ALTARACTIVATIONANIMACOST_HPP__
