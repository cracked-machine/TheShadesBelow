#ifndef SRC_COMPONENTS_PLAYER_TIMEDEFFECT_HPP__
#define SRC_COMPONENTS_PLAYER_TIMEDEFFECT_HPP__

#include <SFML/System/Time.hpp>

namespace Game::Cmp::Player
{

//! @brief  Base for a temporary player effect (e.g. RepelNpc, ToxicImmunity). PlayerSystem accumulates m_timer
//!         and removes the derived component once it expires. Its visual indicator blinks during the last
//!         kWarningTime to warn the player that the effect is about to end.
class TimedEffect
{
public:
  //! @brief Construct with the effect timeout.
  //! @param timeout how long the effect lasts
  TimedEffect( sf::Time timeout )
      : m_timeout( timeout )
  {
  }
  //! @brief Whether the effect has run its course.
  //! @return true if m_timer has reached m_timeout
  [[nodiscard]] bool expired() const { return m_timer >= m_timeout; }
  //! @brief Time left before the effect expires.
  //! @return sf::Time m_timeout - m_timer
  [[nodiscard]] sf::Time remaining() const { return m_timeout - m_timer; }
  //! @brief Whether the effect's visual indicator should be drawn this frame.
  //! @return true, except for the "off" half of each blink during the last kWarningTime
  [[nodiscard]] bool indicator_visible() const
  {
    if ( remaining() > kWarningTime ) return true;
    return static_cast<int>( remaining() / kBlinkHalfPeriod ) % 2 == 0;
  }
  //! @brief Time elapsed since the effect started.
  sf::Time m_timer{ sf::Time::Zero };
  //! @brief How long the effect lasts.
  sf::Time m_timeout;
  //! @brief The indicator blinks for this long before the effect expires.
  static constexpr sf::Time kWarningTime = sf::seconds( 10.f );
  //! @brief Half the blink period of the expiry warning (2 Hz blink).
  static constexpr sf::Time kBlinkHalfPeriod = sf::seconds( 0.25f );
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_TIMEDEFFECT_HPP__
