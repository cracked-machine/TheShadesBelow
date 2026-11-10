#ifndef SRC_COMPONENTS_PERSISTENT_NPCCROWMAXPERCHWAIT_HPP__
#define SRC_COMPONENTS_PERSISTENT_NPCCROWMAXPERCHWAIT_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Longest a perched crow stands still before its next move, in seconds.
struct NpcCrowMaxPerchWait : public BasePersistent<float>
{
  //! @brief Construct with an initial wait and optional min/max bounds.
  //! @param value initial wait, in seconds
  //! @param min_value minimum allowed wait
  //! @param max_value maximum allowed wait
  NpcCrowMaxPerchWait( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "NpcCrowMaxPerchWait"
  virtual std::string class_name() const override { return "NpcCrowMaxPerchWait"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_NPCCROWMAXPERCHWAIT_HPP__
