#ifndef SRC_COMPONENTS_PERSISTENT_BOMBSIGNITEPLANTS_HPP__
#define SRC_COMPONENTS_PERSISTENT_BOMBSIGNITEPLANTS_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Whether bomb blasts set the plants they reach on fire, as if burnt with a torch.
//! @note Read by Sys::BombSystem as each blast cell detonates.
class BombsIgnitePlants : public BasePersistent<bool>
{
public:
  //! @brief Construct the bombs ignite plants setting.
  //! @param value true to let bomb blasts ignite plants.
  BombsIgnitePlants( bool value = true )
      : BasePersistent<bool>( value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "BombsIgnitePlants"; }
  //! @brief Returns the display description for this setting (unset; see persist.json).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_BOMBSIGNITEPLANTS_HPP__
