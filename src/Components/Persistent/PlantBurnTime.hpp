#ifndef SRC_COMPONENTS_PERSISTENT_PLANTBURNTIME_HPP__
#define SRC_COMPONENTS_PERSISTENT_PLANTBURNTIME_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief How long a plant burns for once lit, in seconds. A negative value (-1) means it burns forever.
struct PlantBurnTime : public BasePersistent<float>
{
  //! @brief Construct with an initial burn time and optional min/max bounds.
  //! @param value initial burn time in seconds
  //! @param min_value minimum allowed burn time in seconds
  //! @param max_value maximum allowed burn time in seconds
  PlantBurnTime( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "PlantBurnTime"
  [[nodiscard]] std::string class_name() const override { return "PlantBurnTime"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_PLANTBURNTIME_HPP__
