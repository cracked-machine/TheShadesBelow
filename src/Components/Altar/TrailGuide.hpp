#ifndef SRC_CMP_ALTAR_TRAILGUIDE_HPP__
#define SRC_CMP_ALTAR_TRAILGUIDE_HPP__

#include <Components/Player/Doglegs.hpp>

#include <SFML/System/Time.hpp>

namespace Game::Cmp::Altar
{

//! @brief The chevron paths from the activated altar to each Cmp::Grave::TrailStart grave. Lives on its own entity
//! (with a Cmp::ZOrderValue) that TrailGuideSystem creates, rebuilds every frame and destroys once the trail starts.
//! Drawn by RenderGameSystem.
struct TrailGuide
{
  //! @brief One path per trail start grave
  Cmp::Player::Doglegs paths;
  //! @brief Time since the chevron pulse last left the altar. Advanced and wrapped by TrailGuideSystem.
  sf::Time pulse_time{ sf::Time::Zero };
};

} // namespace Game::Cmp::Altar

#endif // SRC_CMP_ALTAR_TRAILGUIDE_HPP__
