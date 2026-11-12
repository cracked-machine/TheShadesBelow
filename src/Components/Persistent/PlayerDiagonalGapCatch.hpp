#ifndef SRC_COMPONENTS_PERSISTENT_PLAYERDIAGONALGAPCATCH_HPP__
#define SRC_COMPONENTS_PERSISTENT_PLAYERDIAGONALGAPCATCH_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief When the player holds two perpendicular directions while sliding along a wall, stop them exactly
//! aligned with the next 1-cell opening they are pushing towards instead of letting the slide step past it.
//! @note Read by Sys::PlayerSystem::update_player_position.
class PlayerDiagonalGapCatch : public BasePersistent<bool>
{
public:
  //! @brief Construct the diagonal gap catch setting.
  //! @param value true to stop the player at openings while sliding diagonally, false to slide past them.
  PlayerDiagonalGapCatch( bool value = true )
      : BasePersistent<bool>( value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  virtual std::string class_name() const override { return "PlayerDiagonalGapCatch"; }
  //! @brief Returns the display description for this setting (unset; see persist.json).
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_PLAYERDIAGONALGAPCATCH_HPP__
