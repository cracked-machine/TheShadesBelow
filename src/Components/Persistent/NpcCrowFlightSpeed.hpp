#ifndef SRC_COMPONENTS_PERSISTENT_NPCCROWFLIGHTSPEED_HPP__
#define SRC_COMPONENTS_PERSISTENT_NPCCROWFLIGHTSPEED_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Flight speed of a flying crow (fleeing its grave or delivering an item), in grid tiles per second.
struct NpcCrowFlightSpeed : public BasePersistent<float>
{
  //! @brief Construct with an initial speed and optional min/max bounds.
  //! @param value initial speed, in grid tiles/s
  //! @param min_value minimum allowed speed
  //! @param max_value maximum allowed speed
  NpcCrowFlightSpeed( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "NpcCrowFlightSpeed"
  virtual std::string class_name() const override { return "NpcCrowFlightSpeed"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_NPCCROWFLIGHTSPEED_HPP__
