#ifndef SRC_COMPONENTS_PERSISTENT_FUSEDELAY_HPP__
#define SRC_COMPONENTS_PERSISTENT_FUSEDELAY_HPP__

#include <Components/Persistent/BasePersistent.hpp>
#include <SFML/System/Time.hpp>

namespace Game::Cmp::Persist
{

//! @brief Delay, in seconds, between a bomb being armed and it detonating.
class FuseDelay : public BasePersistent<float>
{
public:
  //! @brief Construct with an initial fuse delay and optional min/max bounds.
  //! @param value initial fuse delay, in seconds
  //! @param min_value minimum allowed fuse delay
  //! @param max_value maximum allowed fuse delay
  FuseDelay( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "FuseDelay"
  [[nodiscard]] std::string class_name() const override { return "FuseDelay"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist
#endif // SRC_COMPONENTS_PERSISTENT_FUSEDELAY_HPP__
