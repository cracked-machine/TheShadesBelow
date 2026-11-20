#ifndef SRC_COMPONENTS_PERSISTENT_SINKHOLESPREADPERIOD_HPP__
#define SRC_COMPONENTS_PERSISTENT_SINKHOLESPREADPERIOD_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Seconds between sinkhole spread ticks. Each tick adds at most one new sinkhole cell, see HazardFieldSystem::update_hazard_field().
struct SinkholeSpreadPeriod : public BasePersistent<float>
{
  //! @brief Construct with an initial value and optional min/max bounds.
  //! @param value initial value
  //! @param min_value minimum allowed value
  //! @param max_value maximum allowed value
  SinkholeSpreadPeriod( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "SinkholeSpreadPeriod"
  [[nodiscard]] std::string class_name() const override { return "SinkholeSpreadPeriod"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_SINKHOLESPREADPERIOD_HPP__
