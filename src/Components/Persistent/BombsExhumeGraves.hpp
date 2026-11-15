#ifndef SRC_COMPONENTS_PERSISTENT_BOMBSEXHUMEGRAVES_HPP__
#define SRC_COMPONENTS_PERSISTENT_BOMBSEXHUMEGRAVES_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Whether bomb blasts exhume graves within their radius, chaining into further grave bombs.
//! @note Read by Sys::BombSystem when an epicenter detonates.
class BombsExhumeGraves : public BasePersistent<bool>
{
public:
  //! @brief Construct the bombs exhume graves setting.
  //! @param value true to let bomb blasts exhume graves.
  BombsExhumeGraves( bool value = true )
      : BasePersistent<bool>( value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "BombsExhumeGraves"; }
  //! @brief Returns the display description for this setting (unset; see persist.json).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_BOMBSEXHUMEGRAVES_HPP__
