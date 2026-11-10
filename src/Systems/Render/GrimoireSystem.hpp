#ifndef SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__
#define SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__

#include <Components/Inventory/Grimoire.hpp>
#include <Events/AddInventoryEvent.hpp>
#include <Events/GrimoireItemUpdateEvent.hpp>
#include <Events/GrimoireNpcUpdateEvent.hpp>
#include <Events/ItemPickedUpEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Render/UiData.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Game::Sys
{

//! @brief Toggles and drives the player's grimoire (spellbook) overlay. The NPC and plant pages are in separate tabs
//! (an ImGui tab bar), and each subpage's actions are listed in an ImGui table. Gameplay is paused while the overlay is open.
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

  //! @brief Draw the current grimoire page, if the overlay is open. Pumps the frame's ImGui frame while open.
  //! @param dt Real frame time, for ImGui.
  void render( sf::Time dt );

  //! @brief True while the grimoire overlay is toggled open. Scenes that host the grimoire use this as their pause query.
  [[nodiscard]] bool is_grimoire_overlay_open() const;

private:
  //! @brief Toggle the overlay on GRIMOIRE_TOGGLE and, while open, turn the current tab's pages on GRIMOIRE_PREV/GRIMOIRE_NEXT
  //! (arrow keys or mouse wheel). Both debounced. Opening cancels any bow draw and pauses sound effects; closing resumes them.
  //! The overlay can't be opened while the F3 debug panels are shown.
  //! @param ev
  void on_player_action( Events::PlayerActionEvent ev );

  //! @brief Discover the grimoire page for a picked-up plant: reveal its sprite, name and carry action.
  //! @param ev
  void on_item_pickup_event( const Events::ItemPickedUpEvent &ev );

  //! @brief Discover the grimoire page for an item added directly to the inventory (e.g. foraged by hand).
  //! @param ev
  void on_add_inventory_event( const Events::AddInventoryEvent &ev );

  //! @brief Reveal the sprite and carry action on every grimoire subpage belonging to the item. Triggers
  //! Events::GrimoireDiscoveryEvent if anything was newly revealed.
  //! @param item_key
  void discover_item( const Sys::ItemKey &item_key );

  //! @brief Discover an action on a plant's grimoire page once it has been applied to the player. Triggers
  //! Events::GrimoireDiscoveryEvent the first time the action (or the item) is revealed.
  //! @param ev
  void on_grimoire_item_update_event( const Events::GrimoireItemUpdateEvent &ev );

  //! @brief Discover an action on an NPC's grimoire page once it has been applied to the player, revealing the NPC's
  //! sprite and name. Triggers Events::GrimoireDiscoveryEvent the first time the action (or the NPC) is revealed.
  //! @param ev
  void on_grimoire_npc_update_event( const Events::GrimoireNpcUpdateEvent &ev );

  //! @brief Show `page_idx` of `tab` the next time the grimoire opens. No-op while it is open, so a discovery never
  //! turns the page under the player.
  //! @param tab
  //! @param page_idx
  void show_page_when_closed( Cmp::Grimoire::Tab tab, std::size_t page_idx );

  //! @brief Get the display name of an NPC for the grimoire, from its first sprite.
  //! @param npc_type See res/json/npc.json.
  //! @return std::string The NPC's display name, or `npc_type` itself if it is not in the NpcStore.
  static std::string npc_display_name( const std::string &npc_type );

  //! @brief Get the display text of an action's reward for the grimoire.
  //! @param reward The item, wealth or special altar effect granted by the action.
  //! @return std::string The reward item's display name, the wealth gained, or a description of the special effect.
  static std::string reward_display_name( const Cmp::ActionReward &reward );

  //! @brief Draw the NPCs/Plants tab bar at the layout's "tab_bar" ui_tab, and update m_tab from the open tab.
  //! @param ui_data The layout of the current tab.
  void render_tab_bar( const Render::UiData &ui_data );

  //! @brief Draw one page: SFML outlines, icons, names and page label, then the ImGui action tables.
  //! @tparam PageT Cmp::Grimoire::NpcPage or Cmp::Grimoire::PlantPage.
  //! @param ui_data The page's layout.
  //! @param cache_prefix Prefix of the layout's TextColumn cache keys.
  //! @param page The page.
  //! @param page_count Number of pages in the tab, for the page label.
  template <typename PageT>
  void render_page( const Render::UiData &ui_data, const std::string &cache_prefix, const PageT &page, std::size_t page_count );

  //! @brief Draw the page's outline boxes.
  //! @param ui_data The layout of the page currently shown.
  void render_outlines( const Render::UiData &ui_data );

  //! @brief Draw a subpage's sprite icon.
  //! @param icon The layout's icon.
  //! @param sub_page The subpage the icon belongs to.
  void render_icon( const Render::UiData::Icon &icon, const Cmp::Grimoire::SubPage &sub_page );

  //! @brief Draw the "N of M" page label.
  //! @param ui_data The layout of the page currently shown.
  //! @param cache_prefix Prefix of the layout's TextColumn cache keys.
  //! @param page_count Number of pages in the tab.
  void render_page_label( const Render::UiData &ui_data, const std::string &cache_prefix, std::size_t page_count );

  //! @brief Draw a static text, or a subpage's item or NPC name.
  //! @param text The layout's text.
  //! @param cache_prefix Prefix of the layout's TextColumn cache keys.
  //! @param sub_page The subpage whose name it is, or nullptr for a static text.
  void render_text( const Render::UiData::Text &text, const std::string &cache_prefix, const Cmp::Grimoire::SubPage *sub_page );

  //! @brief Draw a subpage's ImGui action table: one row per entry of `table.rows`, its caption and its effects.
  //! @param table The layout's table.
  //! @param sub_page The subpage the table belongs to.
  void render_action_table( const Render::UiData::Table &table, const Cmp::Grimoire::SubPage &sub_page );

  //! @brief The effect lines of one action on a subpage: its reward, stat changes and side effects, "none" if it was
  //! discovered but does nothing, or "?????" if it is still undiscovered.
  //! @param sub_page
  //! @param action_key e.g. "spawn_action".
  //! @return The lines, never empty.
  std::vector<std::string> action_effect_lines( const Cmp::Grimoire::SubPage &sub_page, const std::string &action_key );

  //! @brief Append an action's reward, in brackets, e.g. "(Elder Flute)".
  //! @param lines
  //! @param action The discovered action.
  //! @return true if a line was added.
  static bool append_reward_lines( std::vector<std::string> &lines, const Cmp::BaseAction &action );

  //! @brief Append one line per non-zero stat and toxidrome change of an action.
  //! @param lines
  //! @param action The discovered action.
  //! @return true if a line was added.
  static bool append_stat_lines( std::vector<std::string> &lines, const Cmp::BaseAction &action );

  //! @brief Append one line per side effect of an action, with its chance of firing.
  //! @param lines
  //! @param action The discovered action.
  //! @return true if a line was added.
  bool append_side_effect_lines( std::vector<std::string> &lines, const Cmp::BaseAction &action );

  //! @brief Get the grimoire component.
  //! @return Pointer to the component (held by the player entity), or nullptr if no player exists.
  Cmp::Grimoire *find_grimoire();

  //! @brief Call fn on every grimoire subpage (across all plant pages) that belongs to the given item.
  //! @param item_key
  //! @param fn Called with the index of the plant page holding the subpage, and the subpage itself.
  void for_each_subpage( const Sys::ItemKey &item_key, const std::function<void( std::size_t, Cmp::Grimoire::SubPage & )> &fn );

  //! @brief Number of pages in a tab.
  //! @param grimoire_cmp
  //! @param tab
  static std::size_t page_count( const Cmp::Grimoire &grimoire_cmp, Cmp::Grimoire::Tab tab );

  //! @brief Time since the overlay was last toggled, used to debounce rapid toggling.
  sf::Time m_toggle_debounce;
  //! @brief Time since the page was last turned, used to debounce rapid page turning.
  sf::Time m_navigate_debounce;

  //! @brief Layout of the plant pages, see res/ui/grimoire_plants.json.
  std::unique_ptr<Render::UiData> m_plant_ui_data;
  //! @brief Layout of the NPC pages, see res/ui/grimoire_npc.json.
  std::unique_ptr<Render::UiData> m_npc_ui_data;

  //! @brief The tab currently shown.
  Cmp::Grimoire::Tab m_tab{ Cmp::Grimoire::Tab::NPCS };
  //! @brief Index of the page shown in each tab, indexed by Cmp::Grimoire::Tab.
  std::array<std::size_t, 2> m_page_idx{};
  //! @brief Select m_tab in the ImGui tab bar on the next frame: ImGui otherwise keeps the tab last clicked.
  bool m_select_tab{ true };
  //! @brief Whether the grimoire pumped an ImGui frame last frame, so the frame it closes on can pump one more to
  //! consume the events forwarded to ImGui while it was open.
  bool m_imgui_active{ false };
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_GRIMOIRESYSTEM_HPP__
