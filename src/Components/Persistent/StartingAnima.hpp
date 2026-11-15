#ifndef SRC_COMPONENTS_PERSISTENT_STARTINGANIMA_HPP__
#define SRC_COMPONENTS_PERSISTENT_STARTINGANIMA_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief The player's anima at the start of the game and at the start of every new level.
//! See Factory::Player::create_player and Utils::Player::reset_stats_to_starting.
class StartingAnima : public BasePersistent<int>
{
public:
  //! @brief Construct with an initial anima value and optional min/max bounds.
  //! @param value initial anima value
  //! @param min_value minimum allowed value
  //! @param max_value maximum allowed value
  StartingAnima( int value = 0, int min_value = 0, int max_value = 0 )
      : BasePersistent<int>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "StartingAnima"
  [[nodiscard]] std::string class_name() const override { return "StartingAnima"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_STARTINGANIMA_HPP__
