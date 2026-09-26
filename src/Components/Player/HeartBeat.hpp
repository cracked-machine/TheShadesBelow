#pragma once

#include <SFML/System/Time.hpp>
#include <algorithm>
#include <cmath>

namespace Game::Cmp::Player
{

class HeartBeat
{
public:
  //! @brief The heartbeat freqeuncy for max tachcardia (excessively fast heartbeat)
  static constexpr float kMaxFreq{ 2.5f };
  //! @brief The heartbeat freqeuncy for max bradycardia (excessively slow heartbeat)
  static constexpr float kMinFreq{ 0.5f };
  //! @brief The heartbeat freqeuncy for normal resting state
  static constexpr float kRestingFreq{ 1.f };

  //! @brief Tracks the toxidrome severity; returns true when a beat should sound this frame.
  //! Silent at the resting rate.
  //! @param dt
  //! @param severity -1..1: positive scales resting -> kMaxFreq (tachycardia), negative scales resting -> kMinFreq (bradycardia)
  [[nodiscard]] bool update( sf::Time dt, float severity )
  {
    const float extreme_freq = ( severity > 0.f ) ? kMaxFreq : kMinFreq;
    const float amount = std::clamp( std::abs( severity ), 0.f, 1.f );
    m_freq = std::lerp( kRestingFreq, extreme_freq, amount ); // resting -> extreme as severity grows

    if ( is_resting() )
    {
      m_beat_timer = sf::Time::Zero;
      return false;
      
    }

    m_beat_timer += dt;
    if ( m_beat_timer < sf::seconds( 1.f ) / m_freq ) { return false; }
    m_beat_timer = sf::Time::Zero;
    return true;
  }

  //! @brief Get the current heartbeat frequency.
  //! @return float
  [[nodiscard]] float freq() const { return m_freq; }

  //! @brief True if heartbeat frequency is at normal resting state.
  //! @return true
  //! @return false
  [[nodiscard]] bool is_resting() const { return m_freq == kRestingFreq; }

private:
  //! @brief The current heartbeat frequency
  float m_freq{ kRestingFreq };

  //! @brief Accumulate delta time, track if the heartbeat needs to trigger again
  sf::Time m_beat_timer{ sf::Time::Zero };
};

} // namespace Game::Cmp::Player
