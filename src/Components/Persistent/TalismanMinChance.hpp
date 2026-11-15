#ifndef SRC_COMPONENTS_PERSISTENT_TALISMANMINCHANCE_HPP__
#define SRC_COMPONENTS_PERSISTENT_TALISMANMINCHANCE_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Lowest chance (percent) of an exhumed grave giving a talisman, however bad the player's luck.
//! See Cmp::Grave::Consequence. Also sizes the level's grave count for the worst case, see LevelGenerator.
class TalismanMinChance : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial chance (percent) and optional min/max bounds.
  //! @param value initial chance (percent)
  //! @param min_value minimum allowed chance (percent)
  //! @param max_value maximum allowed chance (percent)
  TalismanMinChance( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "TalismanMinChance"
  [[nodiscard]] std::string class_name() const override { return "TalismanMinChance"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_TALISMANMINCHANCE_HPP__
