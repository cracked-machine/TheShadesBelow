#ifndef SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__
#define SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__

#include <Events/PlayerActionEvent.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sys
{

//! @brief Toggles and drives the player's grimoire (spellbook) overlay.
class GrimoireSystem : public BaseSystem
{
public:
  //! @brief Construct a new Grimoire System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  GrimoireSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Enable/disable the grimoire overlay components based on toggle state.
  //! @param dt
  void update( sf::Time dt );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief Enable the Cmp::Grimoire overlay on all grimoire entities.
  void enable_grimoire_overlay();

  //! @brief Disable the Cmp::Grimoire overlay on all grimoire entities.
  void disable_grimoire_overlay();

  //! @brief True while the grimoire overlay is toggled open.
  [[nodiscard]] bool is_grimoire_overlay_open() const { return m_grimoire_overlay_open; }

private:
  //! @brief Toggle the overlay open/closed on Events::PlayerActionEvent::GameActions::GRIMOIRE, debounced.
  //! @param ev
  void on_player_action( Events::PlayerActionEvent ev );

  //! @brief True while the grimoire overlay is toggled open.
  bool m_grimoire_overlay_open{ false };

  //! @brief Time since the overlay was last toggled, used to debounce rapid toggling.
  sf::Time overlay_debounce;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__
