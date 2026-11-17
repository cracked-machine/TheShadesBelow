#ifndef SRC_COMPONENTS_PERSISTENT_PLANTSPERTYPE_HPP__
#define SRC_COMPONENTS_PERSISTENT_PLANTSPERTYPE_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Number of plants of each plant type placed at random when a level is generated.
//! See Sys::LevelGenerator::gen_random_plants().
class PlantsPerType : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial count and optional min/max bounds.
  //! @param value initial plants per type
  //! @param min_value minimum allowed plants per type
  //! @param max_value maximum allowed plants per type
  PlantsPerType( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "PlantsPerType"
  [[nodiscard]] std::string class_name() const override { return "PlantsPerType"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_PLANTSPERTYPE_HPP__
