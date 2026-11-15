#ifndef SRC_COMPONENTS_PERSISTENT_DIGGINGDAMAGEPERHIT_HPP__
#define SRC_COMPONENTS_PERSISTENT_DIGGINGDAMAGEPERHIT_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief A percentage of damage that should be done to an item that is hit by player with tools
struct DiggingDamagePerHit : public BasePersistent<uint8_t>
{
  //! @brief Construct with an initial damage-per-hit value and optional min/max bounds.
  //! @param value initial damage-per-hit percentage
  //! @param min_value minimum allowed damage-per-hit percentage
  //! @param max_value maximum allowed damage-per-hit percentage
  DiggingDamagePerHit( uint8_t value = 0, uint8_t min_value = 0, uint8_t max_value = 0 )
      : BasePersistent<uint8_t>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "DiggingDamagePerHit"
  [[nodiscard]] std::string class_name() const override { return "DiggingDamagePerHit"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_DIGGINGDAMAGEPERHIT_HPP__
