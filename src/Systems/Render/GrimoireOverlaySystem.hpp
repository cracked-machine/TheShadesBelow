#ifndef SRC_SYSTEMS_RENDER_GRIMOIREOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_GRIMOIREOVERLAYSYSTEM_HPP__

#include <Components/Inventory/Grimoire.hpp>
#include <Events/AddInventoryEvent.hpp>
#include <Events/GrimoireItemUpdateEvent.hpp>
#include <Events/GrimoireNpcUpdateEvent.hpp>
#include <Events/ItemPickedUpEvent.hpp>
#include <Events/OverlayEvent.hpp>
#include <Systems/Render/BaseOverlaySystem.hpp>
#include <Systems/Render/UiLayout.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Game::Sys
{

//! @brief Draws and drives the player's grimoire (spellbook) overlay. OverlayArbitrationSystem opens and closes it on
//! Events::OverlayEvent GRIMOIRE_TOGGLE. The NPC and plant pages are in separate tabs
//! (an ImGui tab bar), and each subpage's actions are listed in an ImGui table. Gameplay is paused while the overlay is open.
//! While it is closed, hovering the mouse over a plant or NPC in the game area shows its grimoire subpage (its name and
//! actions table) in a compact overlay, see res/ui/grimoire_plant_hover.json, res/ui/grimoire_npc_hover.json and
//! Cmp::Persist::GrimoireHoverFollowsMouse.
class GrimoireOverlaySystem : public BaseOverlaySystem
{
public:
  //! @brief Construct a new Grimoire Overlay System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  GrimoireOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Advance the page-turn debounce timer, then draw the current grimoire page if the overlay is open, otherwise the
  //! hover overlay if hovering is allowed (OverlayArbitrationSystem::hover_allowed()) and the mouse is over an NPC or a
  //! plant (an NPC in front of a plant wins).
  //! @param dt Real frame time.
  void update( sf::Time dt ) override;

  //! @brief True while the grimoire overlay is toggled open. Scenes that host the grimoire use this as their pause query.
  [[nodiscard]] bool is_open() const;

  //! @brief Open the grimoire, cancelling any bow draw and pausing the sound effects. No-op if already open or there is
  //! no grimoire.
  void open();

  //! @brief Close the grimoire and resume the sound effects. No-op if it is not open.
  void close();

private:
  //! @brief While the grimoire is open, turn the current tab's pages on GRIMOIRE_PREV/GRIMOIRE_NEXT (arrow keys or mouse
  //! wheel), debounced.
  //! @param ev
  void on_overlay_event( const Events::OverlayEvent &ev );

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

  //! @brief Draw the open grimoire: its tab bar and the current tab's page, in an invisible ImGui window over the layout's
  //! main_outline.
  //! @param grimoire_cmp
  void render_grimoire( const Cmp::Grimoire &grimoire_cmp );

  //! @brief Draw the hover overlay: the subpage of `page` laid out by `ui_layout`, at its fixed position or beside the mouse
  //! cursor (Cmp::Persist::GrimoireHoverFollowsMouse).
  //! @tparam PageT Cmp::Grimoire::NpcPage or Cmp::Grimoire::PlantPage.
  //! @param ui_layout The hover layout, res/ui/grimoire_npc_hover.json or res/ui/grimoire_plant_hover.json.
  //! @param window_id The ImGui window id.
  //! @param cache_prefix Prefix of the layout's TextColumn cache keys.
  //! @param page The hovered NPC's or plant's page.
  template <typename PageT>
  void render_hover( Render::UiLayout &ui_layout, const char *window_id, const std::string &cache_prefix, const PageT &page );

  //! @brief Find the grimoire page of the plant under the mouse cursor in the game area.
  //! @param grimoire_cmp
  //! @return The page, or nullptr if the mouse is outside the window, not over a plant, or over a plant with no page.
  const Cmp::Grimoire::PlantPage *find_hovered_plant_page( const Cmp::Grimoire &grimoire_cmp ) const;

  //! @brief Find the grimoire page of the visible NPC under the mouse cursor in the game area.
  //! @param grimoire_cmp
  //! @return The page, or nullptr if the mouse is outside the window, not over a visible NPC, or over an NPC with no page.
  const Cmp::Grimoire::NpcPage *find_hovered_npc_page( const Cmp::Grimoire &grimoire_cmp ) const;

  //! @brief The mouse cursor's world-space bounds, if it is inside the window.
  //! @return The bounds, or std::nullopt if the cursor is outside the window: sf::Mouse reports positions outside it too,
  //! which would map to entities off screen.
  std::optional<sf::FloatRect> mouse_bounds_in_window() const;

  //! @brief The anchor offset that moves a hover layout's main_outline beside the mouse cursor: below-right of it,
  //! flipped to the left of / above it where it would leave the window, and clamped to the window.
  //! @param ui_layout The hover layout.
  //! @param window_size
  //! @return The offset to pass to Render::UiLayout::sync_to, or {0,0} if the layout has no main_outline.
  sf::Vector2f hover_anchor( Render::UiLayout &ui_layout, sf::Vector2u window_size );

  //! @brief Begin an invisible ImGui window over the layout's main_outline, or over the whole screen without one. Always
  //! pair with ImGui::End.
  //! @param ui_layout
  //! @param id The ImGui window id, e.g. "##grimoire".
  //! @param extra_flags Flags added to the common grimoire window flags.
  //! @return The result of ImGui::Begin.
  static bool begin_layout_window( const Render::UiLayout &ui_layout, const char *id, int extra_flags );

  //! @brief Draw the NPCs/Plants tab bar at the layout's "tab_bar" ui_tab, and update m_tab from the open tab.
  //! @param ui_layout The layout of the current tab.
  void render_tab_bar( const Render::UiLayout &ui_layout );

  //! @brief Draw one page: SFML outlines, icons, names and page label, then the ImGui action tables.
  //! @tparam PageT Cmp::Grimoire::NpcPage or Cmp::Grimoire::PlantPage.
  //! @param ui_layout The page's layout.
  //! @param cache_prefix Prefix of the layout's TextColumn cache keys.
  //! @param page The page.
  //! @param page_count Number of pages in the tab, for the page label.
  template <typename PageT>
  void render_page( const Render::UiLayout &ui_layout, const std::string &cache_prefix, const PageT &page, std::size_t page_count );

  //! @brief Draw a subpage's sprite icon.
  //! @param icon The layout's icon.
  //! @param sub_page The subpage the icon belongs to.
  void render_icon( const Render::UiLayout::Icon &icon, const Cmp::Grimoire::SubPage &sub_page );

  //! @brief Draw the "N of M" page label.
  //! @param ui_layout The layout of the page currently shown.
  //! @param cache_prefix Prefix of the layout's TextColumn cache keys.
  //! @param page_count Number of pages in the tab.
  void render_page_label( const Render::UiLayout &ui_layout, const std::string &cache_prefix, std::size_t page_count );

  //! @brief Draw a static text, or a subpage's item or NPC name.
  //! @param text The layout's text.
  //! @param cache_prefix Prefix of the layout's TextColumn cache keys.
  //! @param sub_page The subpage whose name it is, or nullptr for a static text.
  void render_text( const Render::UiLayout::Text &text, const std::string &cache_prefix, const Cmp::Grimoire::SubPage *sub_page );

  //! @brief Draw a subpage's ImGui action table: one row per entry of `table.rows`, its caption and its effects.
  //! @param table The layout's table.
  //! @param sub_page The subpage the table belongs to.
  void render_action_table( const Render::UiLayout::Table &table, const Cmp::Grimoire::SubPage &sub_page );

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

  static std::size_t tab_index( Cmp::Grimoire::Tab tab ) { return static_cast<std::size_t>( tab ); }

  //! @brief Map a grimoire_plants.json object name (e.g. "forage_actions") to the page's subpage for its
  //! prefix and the remainder of the name (e.g. "actions").
  //! @return The subpage, or nullptr if the name has no subpage prefix.
  std::pair<const Cmp::Grimoire::SubPage *, std::string_view> resolve_subpage( const Cmp::Grimoire::PlantPage &page, std::string_view name )
  {
    // the *_special_ prefixes must come before plant_/forage_/drop_ as the first matching prefix wins
    const std::array<std::pair<std::string_view, const Cmp::Grimoire::SubPage *>, 6> prefixes = {
        { { "plant_special_", &page.plant_offering_subpage },
          { "forage_special_", &page.forage_offering_subpage },
          { "drop_special_", &page.drop_offering_subpage },
          { "plant_", &page.plant_subpage },
          { "forage_", &page.forage_subpage },
          { "drop_", &page.drop_subpage } } };
    for ( const auto &[prefix, sub_page] : prefixes )
    {
      if ( name.starts_with( prefix ) ) { return { sub_page, name.substr( prefix.size() ) }; }
    }
    return { nullptr, name };
  }

  //! @brief Map a grimoire_npc.json object name (e.g. "npc_actions") to the NPC page's subpage and the remainder
  //! of the name (e.g. "actions").
  //! @return The subpage, or nullptr if the name has no "npc_" prefix.
  static std::pair<const Cmp::Grimoire::SubPage *, std::string_view> resolve_subpage( const Cmp::Grimoire::NpcPage &page, std::string_view name )
  {
    static constexpr std::string_view kPrefix = "npc_";
    if ( name.starts_with( kPrefix ) ) { return { &page.npc_subpage, name.substr( kPrefix.size() ) }; }
    return { nullptr, name };
  }
  //! @brief Tab bar captions, indexed by Cmp::Grimoire::Tab.
  static constexpr std::array<const char *, 2> kTabLabels = { "NPCs", "Plants" };

  //! @brief Number of pages in a tab.
  //! @param grimoire_cmp
  //! @param tab
  static std::size_t page_count( const Cmp::Grimoire &grimoire_cmp, Cmp::Grimoire::Tab tab );

  //! @brief Time since the page was last turned, used to debounce rapid page turning.
  sf::Time m_navigate_debounce;

  //! @brief Layout of the plant pages, see res/ui/grimoire_plants.json.
  std::unique_ptr<Render::UiLayout> m_plant_ui_layout;
  //! @brief Layout of the NPC pages, see res/ui/grimoire_npc.json.
  std::unique_ptr<Render::UiLayout> m_npc_ui_layout;
  //! @brief Layout of the plant hover overlay, see res/ui/grimoire_plant_hover.json.
  std::unique_ptr<Render::UiLayout> m_plant_hover_ui_layout;
  //! @brief Layout of the NPC hover overlay, see res/ui/grimoire_npc_hover.json.
  std::unique_ptr<Render::UiLayout> m_npc_hover_ui_layout;

  //! @brief The tab currently shown.
  Cmp::Grimoire::Tab m_tab{ Cmp::Grimoire::Tab::NPCS };
  //! @brief Index of the page shown in each tab, indexed by Cmp::Grimoire::Tab.
  std::array<std::size_t, 2> m_page_idx{};
  //! @brief Select m_tab in the ImGui tab bar on the next frame: ImGui otherwise keeps the tab last clicked.
  bool m_select_tab{ true };
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_GRIMOIREOVERLAYSYSTEM_HPP__
