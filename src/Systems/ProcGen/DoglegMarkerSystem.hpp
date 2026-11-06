#ifndef SRC_SYSTEMS_PROCGEN_DOGLEGMARKERSYSTEM_HPP__
#define SRC_SYSTEMS_PROCGEN_DOGLEGMARKERSYSTEM_HPP__

#include <Components/Player/Doglegs.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <optional>

namespace Game::Sys::ProcGen
{

//! @brief Base for the guides that send a train of colored chevrons along paths towards a landmark. Derived systems
//! decide which paths to add to a Cmp::Player::Doglegs each frame, then call update_markers() to lay the chevrons
//! along them. See DowsingGuideSystem and TrailGuideSystem.
class DoglegMarkerSystem : public BaseSystem
{
public:
  DoglegMarkerSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
      : BaseSystem( reg, window, sound_bank )
  {
  }

protected:
  //! @brief Add a single L-shaped "crow flies" dogleg from source_pos to target_pos
  //! @param doglegs The paths to add to
  //! @param source_pos Where the chevrons leave from
  //! @param target_pos Where the chevrons travel to
  //! @param color Chevron color
  static void add_dogleg( Cmp::Player::Doglegs &doglegs, sf::Vector2f source_pos, sf::Vector2f target_pos, sf::Color color );

  //! @brief Clear the paths and markers
  static void clear( Cmp::Player::Doglegs &doglegs );

  //! @brief Distance from the path start to where the path leaves the view, or the whole path if the target is in view.
  //! @return 0 if the path starts outside the view
  static float visible_length( const Cmp::Player::Doglegs::Dogleg &dogleg, const sf::FloatRect &view_bounds );

  //! @brief Place the chevron markers along each path for the current pulse time
  //! @param doglegs The paths to lay markers along; its markers are rebuilt
  //! @param pulse_time Time since the pulse last left the path start. Reset here when the pulse restarts.
  //! @param repeat_every std::nullopt: a single train, which only travels the visible part of each path and restarts once
  //! it has cleared the longest. Otherwise a new train leaves every `repeat_every` px and runs the whole length of
  //! each path, on screen or not.
  static void update_markers( Cmp::Player::Doglegs &doglegs, sf::Time &pulse_time, std::optional<float> repeat_every = std::nullopt );
};

} // namespace Game::Sys::ProcGen

#endif // SRC_SYSTEMS_PROCGEN_DOGLEGMARKERSYSTEM_HPP__
