#ifndef SRC_COMPONENTS_PERSISTENT_BOMBDAMAGE_HPP__
#define SRC_COMPONENTS_PERSISTENT_BOMBDAMAGE_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Damage a bomb blast deals to inventory item wear level when it catches the player.
class BombDamage : public BasePersistent<uint8_t>
{
public:
  //! @brief Construct with an initial damage value and optional min/max bounds.
  //! @param value initial damage value
  //! @param min_value minimum allowed damage value
  //! @param max_value maximum allowed damage value
  BombDamage( uint8_t value = 0, uint8_t min_value = 0, uint8_t max_value = 0 )
      : BasePersistent<uint8_t>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "BombDamage"
  [[nodiscard]] std::string class_name() const override { return "BombDamage"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_BOMBDAMAGE_HPP__
