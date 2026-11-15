#ifndef SRC_COMPONENTS_PERSISTENT_BOMBBONUS_HPP__
#define SRC_COMPONENTS_PERSISTENT_BOMBBONUS_HPP__

#include <Components/Persistent/BasePersistent.hpp>
#include <cstdint>
namespace Game::Cmp::Persist
{

//! @brief Bonus number of bombs granted to the player (e.g. via loot).
class BombBonus : public BasePersistent<uint8_t>
{
public:
  //! @brief Construct with an initial bonus amount and optional min/max bounds.
  //! @param value initial bomb bonus amount
  //! @param min_value minimum allowed bonus amount
  //! @param max_value maximum allowed bonus amount
  BombBonus( uint8_t value = 0, uint8_t min_value = 0, uint8_t max_value = 0 )
      : BasePersistent<uint8_t>( value, min_value, max_value )
  {
  }

  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "BombBonus"
  [[nodiscard]] std::string class_name() const override { return "BombBonus"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_BOMBBONUS_HPP__
