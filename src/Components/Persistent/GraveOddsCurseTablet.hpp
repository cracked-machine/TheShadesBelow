#ifndef SRC_COMPONENTS_PERSISTENT_GRAVEODDSCURSETABLET_HPP__
#define SRC_COMPONENTS_PERSISTENT_GRAVEODDSCURSETABLET_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Place of the curse tablet grave consequence on the odds line, 0 = worst and 100 = best.
//! Equal values share a band. See Cmp::Grave::Consequence. Applied at level gen.
class GraveOddsCurseTablet : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial line value and optional min/max bounds.
  //! @param value initial line value
  //! @param min_value minimum allowed line value
  //! @param max_value maximum allowed line value
  GraveOddsCurseTablet( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "GraveOddsCurseTablet"
  [[nodiscard]] virtual std::string class_name() const override { return "GraveOddsCurseTablet"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_GRAVEODDSCURSETABLET_HPP__
