#ifndef SRC_COMPONENTS_PERSISTENT_WORMHOLEANIMFRAMERATE_HPP__
#define SRC_COMPONENTS_PERSISTENT_WORMHOLEANIMFRAMERATE_HPP__

#include <Components/Persistent/BasePersistent.hpp>
#include <SFML/System/Time.hpp>

namespace Game::Cmp::Persist
{

//! @brief Playback rate of the wormhole animation.
class WormholeAnimFramerate : public BasePersistent<float>
{
public:
  //! @brief Construct with an initial framerate and optional min/max bounds.
  //! @note The unit is frames per second. 0 = not animated.
  //! @param framerate initial framerate in frames per second
  //! @param min_value minimum allowed framerate
  //! @param max_value maximum allowed framerate
  WormholeAnimFramerate( float framerate = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( framerate, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "WormholeAnimFramerate"
  virtual std::string class_name() const override { return "WormholeAnimFramerate"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_WORMHOLEANIMFRAMERATE_HPP__
