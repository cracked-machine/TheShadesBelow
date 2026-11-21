#ifndef SRC_COMPONENTS_PERSISTENT_MAXLEVELDEPTH_HPP__
#define SRC_COMPONENTS_PERSISTENT_MAXLEVELDEPTH_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief The deepest level of a run. Completing it ends the run and returns to the title, see LevelCompleteScene.
class MaxLevelDepth : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial max level depth and optional min/max bounds.
  //! @param value initial max level depth
  //! @param min_value minimum allowed max level depth
  //! @param max_value maximum allowed max level depth
  MaxLevelDepth( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "MaxLevelDepth"
  [[nodiscard]] std::string class_name() const override { return "MaxLevelDepth"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_MAXLEVELDEPTH_HPP__
