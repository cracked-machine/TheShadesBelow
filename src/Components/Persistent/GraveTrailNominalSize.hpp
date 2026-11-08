#ifndef SRC_COMPONENTS_PERSISTENT_GRAVETRAILNOMINALSIZE_HPP__
#define SRC_COMPONENTS_PERSISTENT_GRAVETRAILNOMINALSIZE_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Nominal grave trail size. The trail length at a level is this plus the level depth, see GraveSystem.
class GraveTrailNominalSize : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial trail size and optional min/max bounds.
  //! @param value initial trail size
  //! @param min_value minimum allowed trail size
  //! @param max_value maximum allowed trail size
  GraveTrailNominalSize( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "GraveTrailNominalSize"
  [[nodiscard]] virtual std::string class_name() const override { return "GraveTrailNominalSize"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_GRAVETRAILNOMINALSIZE_HPP__
