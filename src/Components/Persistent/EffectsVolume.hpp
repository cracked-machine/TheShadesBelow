#ifndef SRC_COMPONENTS_PERSISTENT_EFFECTSVOLUME_HPP__
#define SRC_COMPONENTS_PERSISTENT_EFFECTSVOLUME_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Volume level for sound effects.
class EffectsVolume : public BasePersistent<float>
{
public:
  //! @brief Construct with an initial volume value and optional min/max bounds.
  //! @param value initial effects volume
  //! @param min_value minimum allowed volume
  //! @param max_value maximum allowed volume
  EffectsVolume( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "EffectsVolume"
  [[nodiscard]] std::string class_name() const override { return "EffectsVolume"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_EFFECTSVOLUME_HPP__
