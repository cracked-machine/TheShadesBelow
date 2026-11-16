#ifndef SRC_COMPONENTS_PERSISTENT_HEALINGBUILDINGOVERRIDE_HPP__
#define SRC_COMPONENTS_PERSISTENT_HEALINGBUILDINGOVERRIDE_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Forces which healing building the level generator places in the graveyard
//! (0 = Healing Spring, 1 = Barber Surgeon). -1 (or any out-of-range value) picks one at random (50/50).
class HealingBuildingOverride : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial building kind and optional min/max bounds.
  //! @param value initial building kind
  //! @param min_value minimum allowed value
  //! @param max_value maximum allowed value
  HealingBuildingOverride( int value = -1, int min_value = -1, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "HealingBuildingOverride"
  [[nodiscard]] std::string class_name() const override { return "HealingBuildingOverride"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_HEALINGBUILDINGOVERRIDE_HPP__
