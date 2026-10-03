#ifndef SRC_COMPONENTS_PERSISTENT_MINNUMCRYPTS_HPP__
#define SRC_COMPONENTS_PERSISTENT_MINNUMCRYPTS_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Minimum number of crypts the level generator guarantees to place in a level.
class MinNumCrypts : public BasePersistent<uint8_t>
{
public:
  //! @brief Construct the min crypt count setting.
  //! @param value Initial minimum crypt count.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  MinNumCrypts( uint8_t value = 0, uint8_t min_value = 0, uint8_t max_value = 0 )
      : BasePersistent<uint8_t>( value, min_value, max_value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  virtual std::string class_name() const override { return "MinNumCrypts"; }
  //! @brief Returns the display description for this setting (unset).
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_MINNUMCRYPTS_HPP__
