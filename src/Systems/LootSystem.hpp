#ifndef SRC_SYSTEMS_LOOTSYSTEM_HPP__
#define SRC_SYSTEMS_LOOTSYSTEM_HPP__

#include <Components/LootContainer.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sys
{

//! @brief Handles player pickup of dropped loot items.
class LootSystem : public BaseSystem
{
public:
  //! @brief Construct a new Loot System object
  //! @param reg
  //! @param window
  //! @param sprite_factory
  //! @param sound_bank
  LootSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief Check for player collision with loot items i.e. player picks up loot
  void check_loot_collision();

private:
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_LOOTSYSTEM_HPP__
