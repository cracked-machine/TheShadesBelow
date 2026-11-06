#ifndef SRC_SYSTEMS_PROCGEN_TRAILGUIDESYSTEM_HPP__
#define SRC_SYSTEMS_PROCGEN_TRAILGUIDESYSTEM_HPP__

#include <Systems/ProcGen/DoglegMarkerSystem.hpp>

#include <SFML/System/Time.hpp>

namespace Game::Sys::ProcGen
{

//! @brief Guides the player from the activated altar to each Cmp::Grave::TrailStart grave with trains of chevrons
//! along L-shaped "crow flies" doglegs. Trains leave the altar continuously and run the whole length of each path, so
//! they are seen arriving at a grave even while the altar is off screen. The guide is shown from the altar's
//! activation until the trail starts. Results are stored in a Cmp::Altar::TrailGuide for RenderGameSystem to draw.
class TrailGuideSystem : public DoglegMarkerSystem
{
public:
  //! @brief Construct a new TrailGuideSystem object
  TrailGuideSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
      : DoglegMarkerSystem( reg, window, sound_bank )
  {
  }

  //! @brief Rebuild the guide for this frame, creating its entity if needed. Destroys the guide entity if no altar is
  //! activated or no trail start graves remain.
  //! @param dt Frame delta, advances the pulse
  void update( sf::Time dt );
};

} // namespace Game::Sys::ProcGen

#endif // SRC_SYSTEMS_PROCGEN_TRAILGUIDESYSTEM_HPP__
