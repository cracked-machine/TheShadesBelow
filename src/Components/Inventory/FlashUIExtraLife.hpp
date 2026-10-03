#ifndef SRC_COMPONENTS_INVENTORY_FLASHUIEXTRALIFE_HPP__
#define SRC_COMPONENTS_INVENTORY_FLASHUIEXTRALIFE_HPP__

namespace Game::Cmp
{

//! @brief Signals to the Render system to flash the UI for player extra life
//! @note This live indefinitely until it is destroyed by Factory::Player::remove_player_extra_life().
class FlashUIExtraLife
{
public:
  sf::Time cooldown_timer{ sf::Time::Zero };
  sf::Time timeout() { return cooldown_timeout; }

private:
  sf::Time cooldown_timeout{ sf::seconds( 0.f ) };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_FLASHUIEXTRALIFE_HPP__
