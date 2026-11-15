#ifndef SRC_COMPONENTS_PERSISTENT_CORRUPTIONDAMAGE_HPP__
#define SRC_COMPONENTS_PERSISTENT_CORRUPTIONDAMAGE_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Damage dealt to the player by the corruption hazard field per tick/exposure.
class CorruptionDamage : public BasePersistent<uint8_t>
{
public:
  //! @brief Construct with an initial damage value and optional min/max bounds.
  //! @param value initial damage value
  //! @param min_value minimum allowed damage value
  //! @param max_value maximum allowed damage value
  CorruptionDamage( uint8_t value = 0, uint8_t min_value = 0, uint8_t max_value = 0 )
      : BasePersistent<uint8_t>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "CorruptionDamage"
  [[nodiscard]] std::string class_name() const override { return "CorruptionDamage"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_CORRUPTIONDAMAGE_HPP__
