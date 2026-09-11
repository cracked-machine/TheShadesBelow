#ifndef SRC_SYSTEMS_ALTARSYSTEM_HPP__
#define SRC_SYSTEMS_ALTARSYSTEM_HPP__

#include <Systems/BaseSystem.hpp>

// clang-format off
namespace Game::Cmp::Altar { class MultiBlock; }
namespace Game::Events { class PlayerActionEvent; }
// clang-format on

namespace Game::Sys
{

//! @brief Handles crypt altar sacrifices, granting the player rewards for items offered up.
class AltarSystem : public BaseSystem
{
public:
  //! @brief Construct a new Altar System object
  //! @param reg
  //! @param window
  //! @param sprite_factory
  //! @param sound_bank
  AltarSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

  //! @brief Per-frame tidy up
  void update( sf::Time dt );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Capture Events::PlayerActionEvent::GameActions::ACTIVATE. Calls check_player_altar_activation().
  void on_player_action( Events::PlayerActionEvent ev );

  //! @brief Rewards player for sacrificing various items at the altar
  //! @param altar_entity
  //! @param altar_cmp
  void check_player_altar_activation( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp );
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_ALTARSYSTEM_HPP__
