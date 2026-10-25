#ifndef SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__
#define SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__

#include <Components/Inventory/Grimoire.hpp>
#include <Events/AddInventoryEvent.hpp>
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

  //! @brief Discover the grimoire page for an item added directly to the inventory (e.g. foraged by hand).
  //! @param ev
  void on_add_inventory_event( const Events::AddInventoryEvent &ev );

  //! @brief Reveal the sprite and carry action on every grimoire subpage belonging to the item.
  //! @param item_key
  void discover_item( const Sys::ItemKey &item_key );

  //! @brief Discover an action on a plant's grimoire page once it has been applied to the player.
  //! @param ev
  void on_grimoire_update_event( const Events::GrimoireUpdateEvent &ev );

  //! @brief Get the display name of an NPC for the grimoire, from its first sprite.
  //! @param npc_type See res/json/npc.json.
  //! @return std::string The NPC's display name, or `npc_type` itself if it is not in the NpcStore.
  static std::string npc_display_name( const std::string &npc_type );

  //! @brief Get the display text of an action's reward for the grimoire.
  //! @param reward The item, wealth or special altar effect granted by the action.
  //! @return std::string The reward item's display name, the wealth gained, or a description of the special effect.
  static std::string reward_display_name( const Cmp::ActionReward &reward );

  //! @brief Draw the page's outline boxes.
  void render_outlines();

  //! @brief Draw each subpage's sprite icon.
  //! @param current_page The page currently shown.
  void render_icons( const Cmp::Grimoire::Page &current_page );

  //! @brief Draw the page number and, for each discovered action, its reward, stat changes and side effects.
  //! @param current_page The page currently shown.
  //! @param page_count Total number of pages, for the page label.
  void render_labels( const Cmp::Grimoire::Page &current_page, std::size_t page_count );

  //! @brief Draw an action's reward, in brackets, e.g. "(Elder Flute)".
  //! @param draw_line The label's text column.
  //! @param action The discovered action.
  //! @return true if a line was drawn.
  bool render_action_reward( TextColumn &draw_line, const Cmp::BaseAction &action );

  //! @brief Draw one line per non-zero stat and toxidrome change of an action.
  //! @param draw_line The label's text column.
  //! @param action The discovered action.
  //! @return true if a line was drawn.
  bool render_action_stats( TextColumn &draw_line, const Cmp::BaseAction &action );

  //! @brief Draw one line per side effect of an action, with its chance of firing.
  //! @param draw_line The label's text column.
  //! @param action The discovered action.
  //! @return true if a line was drawn.
  bool render_action_side_effects( TextColumn &draw_line, const Cmp::BaseAction &action );

  //! @brief Draw the static texts and each subpage's item name.
  //! @param current_page The page currently shown.
  void render_texts( const Cmp::Grimoire::Page &current_page );

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
