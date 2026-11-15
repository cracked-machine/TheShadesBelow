#ifndef SRC_COMPONENTS_PERSISTENT_LUCKDROPRATE_HPP__
#define SRC_COMPONENTS_PERSISTENT_LUCKDROPRATE_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief How many seconds of gameplay it takes for the player to lose 1 luck point. 0 disables the drop.
//! See Sys::PlayerSystem::update.
class LuckDropRate : public BasePersistent<float>
{
public:
  //! @brief Construct a new LuckDropRate object.
  //! @param value Initial rate, in seconds per luck point.
  //! @param min_value Minimum allowed rate.
  //! @param max_value Maximum allowed rate.
  LuckDropRate( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }

  //! @brief Returns the class name used for identification and JSON serialization.
  //! @return std::string The class name.
  [[nodiscard]] std::string class_name() const override { return "LuckDropRate"; }
  //! @brief Returns the display detail string for this setting (currently unused/empty).
  //! @return std::string The detail string.
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_LUCKDROPRATE_HPP__
