#ifndef SRC_COMPONENTS_PERSISTENT_CRYPTNPCSPAWNCOUNT_HPP__
#define SRC_COMPONENTS_PERSISTENT_CRYPTNPCSPAWNCOUNT_HPP__

#include <Components/Persistent/BasePersistent.hpp>
#include <cstdint>

namespace Game::Cmp::Persist
{

//! @brief Number of NPCs to spawn into a crypt's open rooms, capped by the number of rooms available.
class CryptNpcSpawnCount : public BasePersistent<uint16_t>
{
public:
  //! @brief Construct with an initial spawn count and optional min/max bounds.
  //! @param value initial spawn count
  //! @param min_value minimum allowed spawn count
  //! @param max_value maximum allowed spawn count
  CryptNpcSpawnCount( uint16_t value = 0, uint16_t min_value = 0, uint16_t max_value = 0 )
      : BasePersistent<uint16_t>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "CryptNpcSpawnCount"
  [[nodiscard]] std::string class_name() const override { return "CryptNpcSpawnCount"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_CRYPTNPCSPAWNCOUNT_HPP__
