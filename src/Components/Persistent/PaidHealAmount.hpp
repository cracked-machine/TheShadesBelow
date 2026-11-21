#ifndef SRC_COMPONENTS_PERSISTENT_PAIDHEALAMOUNT_HPP__
#define SRC_COMPONENTS_PERSISTENT_PAIDHEALAMOUNT_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief How much one paid heal at the barber surgeon restores of the chosen stat
//! (added to health, or removed from fear/despair/infamy).
class PaidHealAmount : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial heal amount and optional min/max bounds.
  //! @param value initial heal amount
  //! @param min_value minimum allowed value
  //! @param max_value maximum allowed value
  PaidHealAmount( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "PaidHealAmount"
  [[nodiscard]] std::string class_name() const override { return "PaidHealAmount"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_PAIDHEALAMOUNT_HPP__
