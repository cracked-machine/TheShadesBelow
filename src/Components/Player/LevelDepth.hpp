#ifndef SRC_COMPONENTS_PLAYER_LEVELDEPTH_HPP__
#define SRC_COMPONENTS_PLAYER_LEVELDEPTH_HPP__

#include <SFML/System/Time.hpp>

namespace Game::Cmp::Player
{

//! @brief Tracks how many levels deep the player has descended, and the timer for the on-screen depth display.
class LevelDepth
{
public:
  //! @brief Construct with an initial level depth.
  //! @param level initial level depth
  LevelDepth( unsigned int level )
      : m_level( level )
  {
  }
  //! @brief Increase the level depth.
  //! @param count amount to add to the current depth
  void increment_count( unsigned int count )
  {
    m_level += count;
    display_timer = sf::Time::Zero;
  }
  //! @brief Decrease the level depth, clamped at zero.
  //! @param count amount to subtract from the current depth
  void decrement_count( unsigned int count )
  {
    if ( m_level == 0 ) return;
    m_level -= count;
  }

  //! @brief Get the current level depth.
  //! @return unsigned int the current level depth
  [[nodiscard]] unsigned int get_count() const { return m_level; }

  //! @brief How long the depth indicator stays visible on screen after being triggered.
  sf::Time display_cooldown{ sf::seconds( 5 ) };
  //! @brief Timer tracking how long the depth indicator has been displayed.
  sf::Time display_timer;

private:
  //! @brief Current level depth.
  unsigned int m_level;
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_LEVELDEPTH_HPP__
