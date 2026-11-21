#ifndef SRC_COMPONENTS_PERSISTENT_NPCCROWGUIDEAPPROACHDISTANCE_HPP__
#define SRC_COMPONENTS_PERSISTENT_NPCCROWGUIDEAPPROACHDISTANCE_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief How close, in grid cells, the player must come to a perched guide crow before it flies on towards the key grave.
struct NpcCrowGuideApproachDistance : public BasePersistent<float>
{
  //! @brief Construct with an initial distance and optional min/max bounds.
  //! @param value initial distance, in grid cells
  //! @param min_value minimum allowed distance
  //! @param max_value maximum allowed distance
  NpcCrowGuideApproachDistance( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "NpcCrowGuideApproachDistance"
  [[nodiscard]] std::string class_name() const override { return "NpcCrowGuideApproachDistance"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_NPCCROWGUIDEAPPROACHDISTANCE_HPP__
