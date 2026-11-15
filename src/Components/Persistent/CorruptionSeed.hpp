#ifndef SRC_COMPONENTS_PERSISTENT_CORRUPTIONSEED_HPP__
#define SRC_COMPONENTS_PERSISTENT_CORRUPTIONSEED_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Random seed used to grow the corruption hazard field (see Sys::HazardTraits<Cmp::CorruptionCell>).
class CorruptionSeed : public BasePersistent<uint64_t>
{
public:
  //! @brief Construct with an initial seed value, bounded to the full range of uint64_t.
  //! @param value initial seed value
  CorruptionSeed( uint64_t value = 0 )
      : BasePersistent<uint64_t>( value, 0, std::numeric_limits<uint64_t>::max() )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "CorruptionSeed"
  [[nodiscard]] std::string class_name() const override { return "CorruptionSeed"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_CORRUPTIONSEED_HPP__
