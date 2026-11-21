#ifndef SRC_COMPONENTS_PERSISTENT_GRAVECLUSTERGRIDSIZE_HPP__
#define SRC_COMPONENTS_PERSISTENT_GRAVECLUSTERGRIDSIZE_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Number of grave slots along each side of a graveyard grave cluster (an N x N grid of graves). See LevelGenerator::gen_graveyard_exterior_multiblocks().
class GraveClusterGridSize : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial value (grid cells/slots) and optional min/max bounds.
  //! @param value initial value
  //! @param min_value minimum allowed value
  //! @param max_value maximum allowed value
  GraveClusterGridSize( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "GraveClusterGridSize"
  [[nodiscard]] std::string class_name() const override { return "GraveClusterGridSize"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_GRAVECLUSTERGRIDSIZE_HPP__
