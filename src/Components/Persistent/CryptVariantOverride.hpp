#ifndef SRC_COMPONENTS_PERSISTENT_CRYPTVARIANTOVERRIDE_HPP__
#define SRC_COMPONENTS_PERSISTENT_CRYPTVARIANTOVERRIDE_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Forces a specific crypt variant by its registration index in Sys::Crypt::CryptVariantArbiter
//! (0 = Classic, 1 = Open). -1 (or any out-of-range value) picks a variant at random.
class CryptVariantOverride : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial variant index and optional min/max bounds.
  //! @param value initial variant index
  //! @param min_value minimum allowed index
  //! @param max_value maximum allowed index
  CryptVariantOverride( int value = -1, int min_value = -1, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "CryptVariantOverride"
  [[nodiscard]] std::string class_name() const override { return "CryptVariantOverride"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_CRYPTVARIANTOVERRIDE_HPP__
