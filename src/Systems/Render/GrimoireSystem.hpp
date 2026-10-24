#ifndef SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__
#define SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__

#include <Events/PlayerActionEvent.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Render/UiData.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <cstddef>
#include <memory>
#include <vector>

namespace Game::Cmp
{
class Grimoire;
} // namespace Game::Cmp

namespace Game::Sys
{

//! @brief Toggles and drives the player's grimoire (spellbook) overlay.
class GrimoireSystem : public RenderSystem
{
public:
  //! @brief Construct a new Grimoire System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  GrimoireSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Advance the toggle and navigation debounce timers.
  //! @param dt
  void update( sf::Time dt );

  //! @brief Draw the current grimoire page, if the overlay is open.
  void render();

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief True while the grimoire overlay is toggled open.
  [[nodiscard]] bool is_grimoire_overlay_open() const;

private:
  //! @brief Toggle the overlay on GRIMOIRE_TOGGLE and, while open, turn pages on GRIMOIRE_PREV/GRIMOIRE_NEXT. Both debounced.
  //! @param ev
  void on_player_action( Events::PlayerActionEvent ev );

  //! @brief Get the grimoire component.
  //! @return Pointer to the component, or nullptr if no grimoire entity exists.
  Cmp::Grimoire *find_grimoire();

  //! @brief Time since the overlay was last toggled, used to debounce rapid toggling.
  sf::Time m_toggle_debounce;
  //! @brief Time since the page was last turned, used to debounce rapid page turning.
  sf::Time m_navigate_debounce;

  //! @brief Layout data object for the main UI
  std::unique_ptr<Render::UiData> m_ui_data;

  //! @brief The plant items shown in the grimoire, one per page, sorted by key.
  std::vector<ItemKey> m_all_items;
  //! @brief Index into m_all_items of the page currently shown.
  std::size_t m_current_idx{ 0 };
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__
