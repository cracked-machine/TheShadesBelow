#ifndef SRC_COMPONENTS_PERSISTENT_WORMHOLESEED_HPP__
#define SRC_COMPONENTS_PERSISTENT_WORMHOLESEED_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief RNG seed used to generate wormhole placement/shape.
//! @note This component does not use JSON serialization as it is managed internally by the game engine.
class WormholeSeed : public BasePersistent<uint64_t>
{
public:
  //! @brief Construct with an initial seed value.
  //! @param value initial seed value
  WormholeSeed( uint64_t value = 0 )
      : BasePersistent<uint64_t>( value, 0, std::numeric_limits<uint64_t>::max() )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "WormholeSeed"
  [[nodiscard]] std::string class_name() const override { return "WormholeSeed"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_WORMHOLESEED_HPP__
