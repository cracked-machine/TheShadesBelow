#ifndef SRC_COMPONENTS_PERSISTENT_BRADYCARDIAMOVEMENTSPEED_HPP__
#define SRC_COMPONENTS_PERSISTENT_BRADYCARDIAMOVEMENTSPEED_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Player movement speed at maximum bradycardia toxicity (slowed heart). Speed is linearly interpolated
//! between Cmp::Persist::PlayerMovementSpeed (no toxicity) and this value as bradycardia toxicity rises.
class BradycardiaMovementSpeed : public BasePersistent<float>
{
public:
  //! @brief Construct the bradycardia movement speed setting.
  //! @param value Movement speed at maximum bradycardia toxicity.
  //! @param min_value Minimum allowed value (ImGui slider bound).
  //! @param max_value Maximum allowed value (ImGui slider bound).
  BradycardiaMovementSpeed( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  virtual std::string class_name() const override { return "BradycardiaMovementSpeed"; }
  //! @brief Returns the display detail string for this setting.
  const std::string get_detail() const override { return detail; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_BRADYCARDIAMOVEMENTSPEED_HPP__
