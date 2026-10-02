#ifndef SRC_COMPONENTS_INVENTORY_FLASHUIWEALTH_HPP__
#define SRC_COMPONENTS_INVENTORY_FLASHUIWEALTH_HPP__

namespace Game::Cmp
{

//! @brief Signals to the Render system to flash the UI for player wealth
//! @note This is destroyed by the Render system.
class FlashUIWealth
{
public:
  sf::Time cooldown_timer{ sf::Time::Zero };
  sf::Time timeout() { return cooldown_timeout; }

private:
  sf::Time cooldown_timeout{ sf::seconds( 3.f ) };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_FLASHUIWEALTH_HPP__
