#ifndef SRC_COMPONENTS_LASTDIRECTION_HPP__
#define SRC_COMPONENTS_LASTDIRECTION_HPP__

#include <SFML/System/Vector2.hpp>

namespace Game::Cmp
{

//! @brief This is basically the same as Cmp::Direction with the exeception that unlike Cmp::Direction,
//!        Cmp::LastDirection does not get reset when the player stops moving. Therefore, you can use
//!        Cmp::LastDirection to always tell which way the player is facing, even when standing still.
//! @note  {1,0} right, {-1,0} left, {0,-1} up, {0,1} down
class LastDirection : public sf::Vector2f
{
public:
  //! @brief Construct with a zero direction vector.
  LastDirection() = default;

  //! @brief Construct from an explicit direction vector.
  //! @param dir The facing direction to store.
  LastDirection( const sf::Vector2f &dir )
      : sf::Vector2f( dir )
  {
  }
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_LASTDIRECTION_HPP__
