#ifndef SRC_COMPONENTS_PERSISTENT_SHOVELGRAVEDIGWEAR_HPP__
#define SRC_COMPONENTS_PERSISTENT_SHOVELGRAVEDIGWEAR_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Wear the shovel takes each time it hits a grave.
struct ShovelGraveDigWear : public BasePersistent<float>
{
  //! @brief Construct with an initial wear amount and optional min/max bounds.
  //! @param value initial wear amount
  //! @param min_value minimum allowed wear amount
  //! @param max_value maximum allowed wear amount
  ShovelGraveDigWear( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "ShovelGraveDigWear"
  virtual std::string class_name() const override { return "ShovelGraveDigWear"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_SHOVELGRAVEDIGWEAR_HPP__
