#ifndef SRC_COMPONENTS_PERSISTENT_NPCCROWPERCHRANGE_HPP__
#define SRC_COMPONENTS_PERSISTENT_NPCCROWPERCHRANGE_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief How far either side of its grave's centre a perched crow walks, in world px.
struct NpcCrowPerchRange : public BasePersistent<float>
{
  //! @brief Construct with an initial range and optional min/max bounds.
  //! @param value initial range, in world px
  //! @param min_value minimum allowed range
  //! @param max_value maximum allowed range
  NpcCrowPerchRange( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "NpcCrowPerchRange"
  [[nodiscard]] std::string class_name() const override { return "NpcCrowPerchRange"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_NPCCROWPERCHRANGE_HPP__
