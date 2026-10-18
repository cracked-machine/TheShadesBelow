#ifndef SRC_COMPONENTS_PERSISTENT_WORMHOLEABORTTHRESHOLD_HPP__
#define SRC_COMPONENTS_PERSISTENT_WORMHOLEABORTTHRESHOLD_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Seconds into a wormhole jump sequence after which a flying player can no longer abort it (point of no return).
struct WormholeAbortThreshold : public BasePersistent<float>
{
  //! @brief Construct with an initial threshold (seconds) and optional min/max bounds.
  //! @param value initial threshold (seconds)
  //! @param min_value minimum allowed threshold (seconds)
  //! @param max_value maximum allowed threshold (seconds)
  WormholeAbortThreshold( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "WormholeAbortThreshold"
  virtual std::string class_name() const override { return "WormholeAbortThreshold"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_WORMHOLEABORTTHRESHOLD_HPP__
