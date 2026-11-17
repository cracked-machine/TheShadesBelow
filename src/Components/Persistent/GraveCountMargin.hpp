#ifndef SRC_COMPONENTS_PERSISTENT_GRAVECOUNTMARGIN_HPP__
#define SRC_COMPONENTS_PERSISTENT_GRAVECOUNTMARGIN_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Extra graves (percent) generated on top of the level's required grave count, to cover unlucky blood
//! rolls and graves lost to bomb blasts. See LevelGenerator.
class GraveCountMargin : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial margin (percent) and optional min/max bounds.
  //! @param value initial margin (percent)
  //! @param min_value minimum allowed margin (percent)
  //! @param max_value maximum allowed margin (percent)
  GraveCountMargin( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "GraveCountMargin"
  [[nodiscard]] std::string class_name() const override { return "GraveCountMargin"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_GRAVECOUNTMARGIN_HPP__
