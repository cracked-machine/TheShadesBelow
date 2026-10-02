#ifndef SRC_COMPONENTS_LOOT_HPP__
#define SRC_COMPONENTS_LOOT_HPP__

#include <SFML/System/Time.hpp>

namespace Game::Cmp
{

//! @brief Mark an entity as a dropped loot item
class Loot
{
public:
  sf::Time cooldown_timer{ sf::Time::Zero };
  sf::Time timeout() { return cooldown_timeout; }

private:
  sf::Time cooldown_timeout{ sf::seconds( 1.f ) };
};

} // namespace Game::Cmp
#endif // SRC_COMPONENTS_LOOT_HPP__
