#ifndef SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__
#define SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__

#include <Components/Inventory/Grimoire.hpp>
#include <Events/GrimoireUpdateEvent.hpp>
#include <Events/ItemPickedUpEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Render/UiData.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <functional>
#include <memory>

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

  //! @brief Discover the grimoire page for a picked-up plant: reveal its sprite, name and carry action.
  //! @param ev
  void on_item_pickup_event( const Events::ItemPickedUpEvent &ev );

  //! @brief Discover an action on a plant's grimoire page once it has been applied to the player.
  //! @param ev
  void on_grimoire_update_event( const Events::GrimoireUpdateEvent &ev );

  //! @brief Get the display name of an NPC for the grimoire, from its first sprite.
  //! @param npc_type See res/json/npc.json.
  //! @return std::string The NPC's display name, or `npc_type` itself if it is not in the NpcStore.
  static std::string npc_display_name( const std::string &npc_type );

  //! @brief Get the grimoire component.
  //! @return Pointer to the component (held by the player entity), or nullptr if no player exists.
  Cmp::Grimoire *find_grimoire();

  //! @brief Call fn on every grimoire subpage (across all pages) that belongs to the given item.
  //! @param item_key
  //! @param fn Called with the index of the page holding the subpage, and the subpage itself.
  void for_each_subpage( const Sys::ItemKey &item_key, const std::function<void( std::size_t, Cmp::Grimoire::SubPage & )> &fn );

  //! @brief Time since the overlay was last toggled, used to debounce rapid toggling.
  sf::Time m_toggle_debounce;
  //! @brief Time since the page was last turned, used to debounce rapid page turning.
  sf::Time m_navigate_debounce;

  //! @brief Layout data object for the main UI
  std::unique_ptr<Render::UiData> m_ui_data;

  //! @brief Index of the page currently shown.
  std::size_t m_current_page_idx{ 0 };
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__
