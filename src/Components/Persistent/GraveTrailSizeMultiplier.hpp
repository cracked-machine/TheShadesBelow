#ifndef SRC_COMPONENTS_PERSISTENT_GRAVETRAILSIZEMULTIPLIER_HPP__
#define SRC_COMPONENTS_PERSISTENT_GRAVETRAILSIZEMULTIPLIER_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Multiplier controlling the size of grave trails.
class GraveTrailSizeMultiplier : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial multiplier value and optional min/max bounds.
  //! @param value initial multiplier value
  //! @param min_value minimum allowed multiplier value
  //! @param max_value maximum allowed multiplier value
  GraveTrailSizeMultiplier( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "GraveTrailSizeMultiplier"
  [[nodiscard]] virtual std::string class_name() const override { return "GraveTrailSizeMultiplier"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_GRAVETRAILSIZEMULTIPLIER_HPP__
