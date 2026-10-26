#ifndef SRC_COMPONENTS_FOOTSTEPTIMER_HPP__
#define SRC_COMPONENTS_FOOTSTEPTIMER_HPP__

#include <SFML/System/Time.hpp>

namespace Game::Cmp
{

//! @brief Tracks how long a footstep mark entity has existed, used to decide when it should
//!        start fading (see Cmp::FootStepAlpha).
struct FootStepTimer
{
  //! @brief Gameplay time elapsed since the footstep mark was created. Accumulated by FootstepSystem::update().
  sf::Time m_elapsed{};
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_FOOTSTEPTIMER_HPP__
