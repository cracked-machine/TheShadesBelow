#ifndef SRC_SYSTEMS_RENDER_HUDOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_HUDOVERLAYSYSTEM_HPP__

#include <Systems/Render/BaseOverlaySystem.hpp>
#include <Systems/Render/UiLayout.hpp>

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entity/entity.hpp>

#include <Components/Inventory/PlayerInventorySlot.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace Game::Sys
{

//! @brief Draws the HUD layered on top of the game world: meters (ImGui::ProgressBar), labels, icons and texts from res/ui/ui.json, the
//! inventory slot table (an ImGui table at the "inventory_items" ui_table), the screen-space torch particles over their
//! inventory slot icons, the crypt maze shuffle countdown and the level depth banner.
class HudOverlaySystem : public BaseOverlaySystem
{
public:
  //! @brief Construct a new HUD Overlay System object. Loads the HUD layout from res/ui/ui.json.
  //! @param reg
  //! @param window
  //! @param sound_bank
  HudOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
      : BaseOverlaySystem( reg, window, sound_bank ),
        m_main_ui_layout( std::make_unique<Render::UiLayout>( "res/ui/ui.json" ) ),
        m_slot_ui_layout( std::make_unique<Render::UiLayout>( kInventorySlotLayout ) )
  {
  }

  //! @brief Draw the HUD: screen-space particles, outlines, icons, meters, labels, texts and the crypt maze timer.
  //! @param dt World frame time (zero while paused), so meter flashes and stat-change labels freeze with the world.
  void update( sf::Time dt ) override;

  //! @brief Render the current dungeon level depth text while its display cooldown has not yet elapsed.
  //! Not part of update(): only GraveyardScene advances the display timer, so other scenes would show it frozen.
  void render_level_depth();

private:
  //! @brief Draw the screen-space particle sprites, i.e. the flame of each lit torch (Utils::Player::is_torch_slot_lit)
  //! flickering over its inventory slot icon. Uses the slot icons recorded by render_ui_inventory_table().
  void render_ui_particles();

  //! @brief Render the HUD's outline rectangles (e.g. panel borders).
  void render_ui_outlines();

  //! @brief Render the HUD's icons, excluding the cadaver icons which are handled by render_ui_cadaver_icons.
  void render_ui_icons();

  //! @brief Render the cadaver part icons that the player has collected.
  void render_ui_cadaver_icons();

  //! @brief Find the "inventory_items" ui_table in the HUD layout.
  //! @return The table, or nullptr if the layout has none.
  [[nodiscard]] const Render::UiLayout::Table *find_inventory_table() const;

  //! @brief The on-screen size of one inventory slot: the res/ui/inventory_slot.json map size at the HUD's scale.
  [[nodiscard]] sf::Vector2f inventory_slot_size() const;

  //! @brief Resize the "inventory_outline" so it keeps its authored padding (its offset to the "inventory_items" table)
  //! around every inventory slot.
  void fit_inventory_outline();

  //! @brief Render every inventory slot as one row of an ImGui table at the "inventory_items" ui_table's position: one
  //! column per slot (column i = slot i), each laid out by res/ui/inventory_slot.json. The selected slot is highlighted.
  //! @note ParticleSprites are rendered seperately in render_ui_particles
  //! @param dt for the Cmp::FlashUIInventory highlight flash
  void render_ui_inventory_table( sf::Time dt );

  //! @brief Render one inventory slot's table cell from the res/ui/inventory_slot.json layout: the selection highlight
  //! (slot_highlight), item icon fitted to slot_icon, slot_number and slot_name text, and the wear level as an
  //! ImGui::ProgressBar at slot_wear. Must be called inside the inventory table, with the cell's column set.
  //! @param idx the slot index
  //! @param selected whether this is the selected slot
  //! @param flash whether the selected slot's highlight is in the "on" phase of a Cmp::FlashUIInventory flash
  //! @param slot_origin screen position of the slot's top left corner
  void render_inventory_slot( uint8_t idx, bool selected, bool flash, sf::Vector2f slot_origin );

  //! @brief Draw ImGui text at a slot layout ui_label: its font_size, and its point aligned per its align property.
  //! @param label the slot layout label
  //! @param slot_origin screen position of the slot's top left corner
  //! @param text the text to draw
  void place_imgui_text( const Render::UiLayout::Label &label, sf::Vector2f slot_origin, const std::string &text );

  //! @brief Render the HUD's meters (health, fear, infamy, despair, luck, anima, toxicity) as ImGui::ProgressBars and
  //! flash their outline colour on recent changes. Must be called inside the "##hud_meters" window (begin_overlay_window).
  //! @param dt
  void render_ui_meters( sf::Time dt );

  //! @brief Lazily load res/shaders/Frost.frag (Cmp::Player::ToxicImmunity "frozen" meters) on first use.
  //! @return true if the shader is available and loaded. A failure is logged once and cached.
  bool load_frost_shader();

  //! @brief Draw the frost shader over a toxidrome meter: render it into the meter's own texture in m_frost_textures,
  //! then show that with ImGui::Image, so it draws over the meter's ProgressBar.
  //! @param meter_name the meter's layout name, which keys its frost texture
  //! @param rect the meter's bounds (outline included), in default view coordinates
  //! @param seed per-meter offset so each meter gets its own frost pattern
  //! @param frost base wash colour
  //! @param glint crystal ridge and glint colour
  void render_meter_frost( const std::string &meter_name, const sf::FloatRect &rect, float seed, sf::Color frost, sf::Color glint );

  //! @brief Render the HUD's static text elements as outlined ImGui text, after the meters in the "##hud_meters"
  //! window so texts over a meter (the toxidrome names) draw on top of it.
  void render_ui_texts();

  //! @brief Render the HUD's dynamic labels (cadaver count, wealth, stat changes) as ImGui text and flash their colour on
  //! recent changes.
  //! @param dt
  void render_ui_labels( sf::Time dt );

  //! @brief Render a red vertical meter over the centre of each on-screen graveyard altar still filling its blood quota
  //! (Cmp::Altar::BloodCount of Cmp::Persist::AltarBloodQuota). Hidden while the altar is activated. Must be called
  //! inside an overlay window.
  void render_altar_blood_meters();

  //! @brief Render a red ImGui::ProgressBar along the top of each on-screen plant and NPC whose Cmp::Health is below 100.
  //! Hidden when Cmp::Persist::ShowHealthBar is off. Must be called inside an overlay window.
  void render_world_health_bars();

  //! @brief Render each Cmp::DamagePopup as a red "-N" rising and fading above where the hit landed, then destroy it once
  //! it has faded. Hidden (but still aged) when Cmp::Persist::ShowDamageText is off. Must be called inside an overlay window.
  //! @param dt Frame time, to age the popups.
  void render_damage_popups( sf::Time dt );

  //! @brief Render the halo effect around the graves that are marked as ghost spawners. Only active when player is carrying ElderFlute.
  void render_ghost_grave_halos();

  //! @brief Render the countdown until the crypt maze next shuffles, if the shuffle timer has not already expired.
  //! @param pos
  //! @param size
  void render_crypt_maze_timer( sf::Vector2f pos, unsigned int size );

  template <typename FlashComponent>
  bool update_flash_toggle( sf::Time dt );

  //! @brief The HUD layout's current design-to-screen scale, for hard-coded HUD pixel values (outline widths, font sizes).
  [[nodiscard]] float hud_scale() const { return ui_scale( m_main_ui_layout.get() ); }

  //! @brief Get the current value of the player stat or toxidrome named by a "<stat>_delta_lbl" UI label.
  //! @param label_name the UI label name, e.g. "health_delta_lbl" or "venom_delta_lbl"
  //! @return std::optional<int> the stat's current value (an inactive toxidrome reads as 0), or nullopt if the name is unknown
  std::optional<int> get_change_label_stat( const std::string &label_name );

  //! @brief Track the stat behind a "<stat>_delta_lbl" UI label and return its recent change as text.
  //! A change shows as a signed delta (e.g. "+5", "-3") for Cmp::Persist::StatChangeDisplayDuration seconds; further
  //! changes while it is showing accumulate into the delta and restart the duration.
  //! @param label_name the UI label name
  //! @param dt
  //! @return std::string the delta text, or an empty string once the duration has elapsed
  std::string update_stat_change_text( const std::string &label_name, sf::Time dt );

  //! @brief Layout data object for the HUD
  std::unique_ptr<Render::UiLayout> m_main_ui_layout;

  //! @brief Layout data object for one inventory slot (res/ui/inventory_slot.json), at the HUD's scale with a {0,0}
  //! origin; slots add their own origin when drawn
  std::unique_ptr<Render::UiLayout> m_slot_ui_layout;

  //! @brief Name of the ui_table the inventory slots are drawn at
  static constexpr const char *kInventoryTableName = "inventory_items";
  //! @brief Name of the ui_outline that is resized to fit the inventory slots
  static constexpr const char *kInventoryOutlineName = "inventory_outline";
  //! @brief Tiled layout of one inventory slot; its map size is the slot size
  static constexpr const char *kInventorySlotLayout = "res/ui/inventory_slot.json";
  //! @brief res/ui/inventory_slot.json ui_outline drawn behind the selected slot
  static constexpr const char *kSlotHighlightName = "slot_highlight";
  //! @brief res/ui/inventory_slot.json ui_icon rect the item sprite is fitted into
  static constexpr const char *kSlotIconName = "slot_icon";
  //! @brief res/ui/inventory_slot.json ui_label for the item name
  static constexpr const char *kSlotNameName = "slot_name";
  //! @brief res/ui/inventory_slot.json ui_label for the slot number
  static constexpr const char *kSlotNumberName = "slot_number";
  //! @brief res/ui/inventory_slot.json ui_label for the stack count of a stackable item (Cmp::Inventory::StackCount)
  static constexpr const char *kSlotCountName = "slot_count";
  //! @brief res/ui/inventory_slot.json ui_meter for the wear level ImGui::ProgressBar
  static constexpr const char *kSlotWearName = "slot_wear";

  //! @brief Where an inventory slot's item icon was drawn this frame
  struct SlotIcon
  {
    //! @brief Screen position of the sprite's top left corner
    sf::Vector2f position;
    //! @brief Uniform scale the sprite was drawn at
    float scale{ 1.f };
  };
  //! @brief Each slot's icon from the last render_ui_inventory_table(), indexed by slot; unset for empty slots
  std::array<std::optional<SlotIcon>, Cmp::kMaxInventorySlots> m_slot_icons{};

  //! @brief Screen flash frequency
  int m_ui_flash_factor{ 300 };

  //! @brief Display state for one "<stat>_delta_lbl" UI label, see update_stat_change_text()
  struct StatChange
  {
    //! @brief The stat's value when last checked
    int last_value{ 0 };
    //! @brief Accumulated change shown in the label
    int delta{ 0 };
    //! @brief Time the current delta has been shown for
    sf::Time elapsed{ sf::Time::Zero };
  };
  //! @brief Per-label change state, keyed by UI label name
  std::unordered_map<std::string, StatChange> m_stat_changes;
  //! @brief Player entity the m_stat_changes baselines were taken from; a new player re-baselines instead of showing a change
  entt::entity m_stat_changes_player{ entt::null };

  //! @brief Frost overlay shader for the toxidrome meters, see load_frost_shader()
  sf::Shader m_frost_shader;
  //! @brief Result of the lazy frost shader load; unset until first use
  std::optional<bool> m_frost_shader_ok;
  //! @brief Drives the frost shader's glint twinkle
  sf::Time m_frost_timer;
  //! @brief Per toxidrome meter frost texture, keyed by meter name, see render_meter_frost()
  std::unordered_map<std::string, sf::RenderTexture> m_frost_textures;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_HUDOVERLAYSYSTEM_HPP__
