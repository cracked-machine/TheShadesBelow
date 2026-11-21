#ifndef SRC_COMPONENTS_PERSISTENT_GRAVECLUSTEREXTERNALPADDING_HPP__
#define SRC_COMPONENTS_PERSISTENT_GRAVECLUSTEREXTERNALPADDING_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Empty grid cells between neighbouring graveyard grave clusters. See LevelGenerator::gen_graveyard_exterior_multiblocks().
class GraveClusterExternalPadding : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial value (grid cells/slots) and optional min/max bounds.
  //! @param value initial value
  //! @param min_value minimum allowed value
  //! @param max_value maximum allowed value
  GraveClusterExternalPadding( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "GraveClusterExternalPadding"
  [[nodiscard]] std::string class_name() const override { return "GraveClusterExternalPadding"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_GRAVECLUSTEREXTERNALPADDING_HPP__
