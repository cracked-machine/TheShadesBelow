#ifndef SRC_SYSTEMS_ANIMSYSTEM_HPP__
#define SRC_SYSTEMS_ANIMSYSTEM_HPP__

#include <SFML/System/Time.hpp>

#include <Components/AnimData.hpp>
#include <Components/Direction.hpp>
#include <Components/Position.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sys
{

//! @brief Advances sprite sheet animation frames each frame based on each entity's Cmp::AnimData.
class AnimSystem : public BaseSystem
{
public:
  //! @brief Construct a new Anim System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  AnimSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
      : BaseSystem( reg, window, sound_bank )
  {
    SPDLOG_DEBUG( "AnimSystem initialized" );
  }

  //! @brief Update frames on all SpriteAnimation components
  //! @param dt
  void update( sf::Time dt );

private:
  //! @brief Update a single animation sequence. Used for tilesets that consist of a single animation sequence.
  //! @param anim
  //! @param globalDeltaTime
  //! @param ms
  //! @param framerate Frames per second. 0 (or less) = not animated.
  static void update_sequence_frame( Cmp::AnimData &anim, sf::Time globalDeltaTime, const Sprites::SpriteSheet &ms, float framerate );
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_ANIMSYSTEM_HPP__
