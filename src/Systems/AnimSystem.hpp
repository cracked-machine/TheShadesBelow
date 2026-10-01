#ifndef SRC_SYSTEMS_ANIMSYSTEM_HPP__
#define SRC_SYSTEMS_ANIMSYSTEM_HPP__

#include <Events/PauseClocksEvent.hpp>
#include <Events/ResumeClocksEvent.hpp>
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
    // The entt::dispatcher is independent of the registry, so it is safe to bind event handlers in the constructor
    std::ignore = get_systems_event_queue().sink<Events::PauseClocksEvent>().connect<&Sys::AnimSystem::on_pause>( this );
    std::ignore = get_systems_event_queue().sink<Events::ResumeClocksEvent>().connect<&Sys::AnimSystem::on_resume>( this );
    SPDLOG_DEBUG( "AnimSystem initialized" );
  }

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief Update frames on all SpriteAnimation components
  //! @param dt
  void update( sf::Time dt );

private:
  //! @brief Update a single animation sequence. Used for tilesets that consist of a single animation sequence.
  //! @param anim
  //! @param globalDeltaTime
  //! @param ms
  //! @param frame_rate
  static void update_sequence_frame( Cmp::AnimData &anim, sf::Time globalDeltaTime, const Sprites::SpriteSheet &ms, sf::Time frame_rate );
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_ANIMSYSTEM_HPP__
