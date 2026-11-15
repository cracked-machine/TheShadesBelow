#ifndef SRC_COMPONENTS_PERSISTENT_NUMALTARS_HPP__
#define SRC_COMPONENTS_PERSISTENT_NUMALTARS_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Number of relic altars the level generator places in a graveyard level. Only one altar can be activated at
//! a time, so this changes how far the player travels between grave trails, not how many graves are needed.
class NumAltars : public BasePersistent<uint8_t>
{
public:
  //! @brief Construct the altar count setting.
  //! @param value Initial altar count.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  NumAltars( uint8_t value = 0, uint8_t min_value = 0, uint8_t max_value = 0 )
      : BasePersistent<uint8_t>( value, min_value, max_value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "NumAltars"; }
  //! @brief Returns the display description for this setting (unset).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_NUMALTARS_HPP__
