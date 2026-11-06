#ifndef SRC_COMPONENTS_PERSISTENT_GRAVETRAILSTARTCOUNT_HPP__
#define SRC_COMPONENTS_PERSISTENT_GRAVETRAILSTARTCOUNT_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief How many graves are offered as the start of the key grave trail when an altar is activated.
class GraveTrailStartCount : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial start count and optional min/max bounds.
  //! @param value initial start count
  //! @param min_value minimum allowed start count
  //! @param max_value maximum allowed start count
  GraveTrailStartCount( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "GraveTrailStartCount"
  [[nodiscard]] virtual std::string class_name() const override { return "GraveTrailStartCount"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_GRAVETRAILSTARTCOUNT_HPP__
