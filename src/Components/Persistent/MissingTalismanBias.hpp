#ifndef SRC_COMPONENTS_PERSISTENT_MISSINGTALISMANBIAS_HPP__
#define SRC_COMPONENTS_PERSISTENT_MISSINGTALISMANBIAS_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief When a grave drops a talisman, the extra weight per talisman already offered at a partly activated altar that is
//! given to each talisman that altar still needs. 0 makes every talisman equally likely.
class MissingTalismanBias : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial bias and optional min/max bounds.
  //! @param value initial bias
  //! @param min_value minimum allowed bias
  //! @param max_value maximum allowed bias
  MissingTalismanBias( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "MissingTalismanBias"
  [[nodiscard]] virtual std::string class_name() const override { return "MissingTalismanBias"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_MISSINGTALISMANBIAS_HPP__
