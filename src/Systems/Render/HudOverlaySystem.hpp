#ifndef SRC_SYSTEMS_RENDER_HUDOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_HUDOVERLAYSYSTEM_HPP__

#include <Systems/Render/BaseOverlaySystem.hpp>
#include <Systems/Render/UiData.hpp>

#include <SFML/Graphics/Shader.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entity/entity.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace Game::Sys
{

//! @brief Draws the HUD layered on top of the game world: meters, labels, icons and texts from res/ui/ui.json, the
//! screen-space torch particles over the inventory icon, the crypt maze shuffle countdown and the level depth banner.
class HudOverlaySystem : public BaseOverlaySystem
{
public:
  //! @brief Construct a new HUD Overlay System object. Loads the HUD layout from res/ui/ui.json.
  //! @param reg
  //! @param window
  //! @param sound_bank
  HudOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
      : BaseOverlaySystem( reg, window, sound_bank ),
        m_main_ui_data( std::make_unique<Render::UiData>( "res/ui/ui.json" ) )
  {
  }

  //! @brief Draw the HUD: screen-space particles, outlines, icons, meters, labels, texts and the crypt maze timer.
  //! @param dt World frame time (zero while paused), so meter flashes and stat-change labels freeze with the world.
  void update( sf::Time dt ) override;

  //! @brief Render the current dungeon level depth text while its display cooldown has not yet elapsed.
  //! Not part of update(): only GraveyardScene advances the display timer, so other scenes would show it frozen.
  void render_level_depth();

private:
  //! @brief Draw the screen-space particle sprites, i.e. the torch flame flickering over the UI inventory icon.
  void render_ui_particles();

  //! @brief Render the HUD's outline rectangles (e.g. panel borders).
  void render_ui_outlines();

  //! @brief Render the HUD's icons, excluding the inventory icon which is handled by render_ui_inventory_icon.
  void render_ui_icons();

  //! @brief Render the cadaver part icons that the player has collected.
  void render_ui_cadaver_icons();

  //! @brief Render the sprite in the Inventory UI.
  //! @note ParticleSprites are rendered seperately in render_ui_particles
  void render_ui_inventory_icon();

  //! @brief Render the HUD's meters (health, fear, infamy, despair, toxicity, inventory wear) and flash their outline colour on
  //! recent changes.
  //! @param dt
  void render_ui_meters( sf::Time dt );

  //! @brief Draw a diagonal shimmer band swept across a meter (Cmp::Player::ToxicImmunity "frozen" meters).
  //! @param rect the meter's bounds (outline included); the band is clipped to it
  //! @param progress sweep position in [0, 1]; the band is off-meter at both ends
  //! @param colour the band's peak colour (its alpha fades to 0 at the band's edges)
  void render_meter_shimmer( const sf::FloatRect &rect, float progress, sf::Color colour );

  //! @brief Lazily load res/shaders/Frost.frag (Cmp::Player::ToxicImmunity "frozen" meters) on first use.
  //! @return true if the shader is available and loaded. A failure is logged once and cached.
  bool load_frost_shader();

  //! @brief Draw the frost shader over a toxidrome meter.
  //! @param rect the meter's bounds (outline included), in default view coordinates
  //! @param seed per-meter offset so each meter gets its own frost pattern
  //! @param frost base wash colour
  //! @param glint crystal ridge and glint colour
  void render_meter_frost( const sf::FloatRect &rect, float seed, sf::Color frost, sf::Color glint );

  //! @brief Render the HUD's static text elements.
  void render_ui_texts();

  //! @brief Render the HUD's dynamic labels (cadaver count, wealth, current inventory item) and flash their colour on
  //! recent changes.
  //! @param dt
  void render_ui_labels( sf::Time dt );

  //! @brief Render the halo effect around the graves that are marked as ghost spawners. Only active when player is carrying ElderFlute.
  void render_ghost_grave_halos();

  //! @brief Render the countdown until the crypt maze next shuffles, if the shuffle timer has not already expired.
  //! @param pos
  //! @param size
  void render_crypt_maze_timer( sf::Vector2f pos, unsigned int size );

  template <typename FlashComponent>
  bool update_flash_toggle( sf::Time dt );

  //! @brief The HUD layout's current design-to-screen scale, for hard-coded HUD pixel values (outline widths, font sizes).
  [[nodiscard]] float hud_scale() const { return ui_scale( m_main_ui_data.get() ); }

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
  std::unique_ptr<Render::UiData> m_main_ui_data;

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
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_HUDOVERLAYSYSTEM_HPP__
