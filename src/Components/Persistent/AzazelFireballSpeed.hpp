#ifndef SRC_COMPONENTS_PERSISTENT_AZAZELFIREBALLSPEED_HPP__
#define SRC_COMPONENTS_PERSISTENT_AZAZELFIREBALLSPEED_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Flight speed (world px/s) of the fireballs Azazel shoots at the player.
struct AzazelFireballSpeed : public BasePersistent<float>
{
  //! @brief Construct with an initial speed and optional min/max bounds.
  //! @param value initial speed, in world px/s
  //! @param min_value minimum allowed speed
  //! @param max_value maximum allowed speed
  AzazelFireballSpeed( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "AzazelFireballSpeed"
  virtual std::string class_name() const override { return "AzazelFireballSpeed"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_AZAZELFIREBALLSPEED_HPP__
