#ifndef SRC_EVENTS_UPDATEPLAYERPOSITIONEVENT_HPP__
#define SRC_EVENTS_UPDATEPLAYERPOSITIONEVENT_HPP__

#include <SFML/System/Vector2.hpp>

namespace Game::Events
{

//! @brief Move the player to a new world position.
//! @note Consumed by PlayerSystem, which is the only place allowed to write the player's Cmp::Position: it keeps
//! the navmeshes in sync with the move. Trigger this instead of modifying the component directly.
class UpdatePlayerPositionEvent
{
public:
  //! @param position The player's new top-left world position.
  explicit UpdatePlayerPositionEvent( sf::Vector2f position )
      : m_position( position )
  {
  }
  sf::Vector2f m_position;
};

} // namespace Game::Events

#endif // SRC_EVENTS_UPDATEPLAYERPOSITIONEVENT_HPP__
