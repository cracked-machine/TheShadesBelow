#ifndef SRC_COMPONENTS_PERSISTENT_GRAVEDROPBOMBTRAPRANK_HPP__
#define SRC_COMPONENTS_PERSISTENT_GRAVEDROPBOMBTRAPRANK_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Rank of the bomb trap grave consequence on the drop line, 0 = worst and 100 = best; -1 removes it from the line.
//! Equal values share a band. See Cmp::Grave::Consequence. Applied at level gen.
class GraveDropBombTrapRank : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial line value and optional min/max bounds.
  //! @param value initial line value
  //! @param min_value minimum allowed line value
  //! @param max_value maximum allowed line value
  GraveDropBombTrapRank( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "GraveDropBombTrapRank"
  [[nodiscard]] std::string class_name() const override { return "GraveDropBombTrapRank"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_GRAVEDROPBOMBTRAPRANK_HPP__
