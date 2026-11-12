#ifndef SRC_COMPONENTS_PERSISTENT_GRAVETRAILNOMINALHOPDISTANCE_HPP__
#define SRC_COMPONENTS_PERSISTENT_GRAVETRAILNOMINALHOPDISTANCE_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Nominal max distance, in grid cells, between consecutive trail graves. The max hop at a level is this plus the
//! level depth, see GraveSystem.
class GraveTrailNominalHopDistance : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial hop distance (grid cells) and optional min/max bounds.
  //! @param value initial hop distance (grid cells)
  //! @param min_value minimum allowed hop distance
  //! @param max_value maximum allowed hop distance
  GraveTrailNominalHopDistance( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "GraveTrailNominalHopDistance"
  [[nodiscard]] virtual std::string class_name() const override { return "GraveTrailNominalHopDistance"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_GRAVETRAILNOMINALHOPDISTANCE_HPP__
