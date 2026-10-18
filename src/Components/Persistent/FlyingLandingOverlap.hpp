#ifndef SRC_COMPONENTS_PERSISTENT_FLYINGLANDINGOVERLAP_HPP__
#define SRC_COMPONENTS_PERSISTENT_FLYINGLANDINGOVERLAP_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Fraction of the player's size, per side, allowed to overlap an obstacle when landing. Within it the player lands on the nearest free grid cell.
struct FlyingLandingOverlap : public BasePersistent<float>
{
  //! @brief Construct with an initial overlap fraction and optional min/max bounds.
  //! @param value initial overlap fraction
  //! @param min_value minimum allowed overlap fraction
  //! @param max_value maximum allowed overlap fraction
  FlyingLandingOverlap( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "FlyingLandingOverlap"
  virtual std::string class_name() const override { return "FlyingLandingOverlap"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_FLYINGLANDINGOVERLAP_HPP__
