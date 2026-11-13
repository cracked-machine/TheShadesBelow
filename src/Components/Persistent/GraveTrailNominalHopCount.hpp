#ifndef SRC_COMPONENTS_PERSISTENT_GRAVETRAILNOMINALHOPCOUNT_HPP__
#define SRC_COMPONENTS_PERSISTENT_GRAVETRAILNOMINALHOPCOUNT_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Nominal number of graves in the grave trail that leads to the exit key. The trail length at a level is this
//! plus the level depth, see GraveSystem.
class GraveTrailNominalHopCount : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial hop count and optional min/max bounds.
  //! @param value initial hop count
  //! @param min_value minimum allowed hop count
  //! @param max_value maximum allowed hop count
  GraveTrailNominalHopCount( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "GraveTrailNominalHopCount"
  [[nodiscard]] virtual std::string class_name() const override { return "GraveTrailNominalHopCount"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_GRAVETRAILNOMINALHOPCOUNT_HPP__
