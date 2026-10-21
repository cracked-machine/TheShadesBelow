#ifndef SRC_COMPONENTS_PERSISTENT_RESETTUTORIAL_HPP__
#define SRC_COMPONENTS_PERSISTENT_RESETTUTORIAL_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Settings menu request to show all "once" tooltips again.
//! @note Sys::TooltipSystem clears Cmp::Persist::TooltipsSeen when this is true, then sets it back to false.
class ResetTutorial : public BasePersistent<bool>
{
public:
  //! @brief Construct the reset tutorial setting.
  //! @param value Initial state of the reset request.
  ResetTutorial( bool value = false )
      : BasePersistent<bool>( value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  virtual std::string class_name() const override { return "ResetTutorial"; }
  //! @brief Returns the display description for this setting (unset).
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_RESETTUTORIAL_HPP__
