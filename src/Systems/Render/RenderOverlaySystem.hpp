#ifndef SRC_SYSTEMS_RENDER_RENDEROVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_RENDEROVERLAYSYSTEM_HPP__

#include <Components/Position.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Render/UiData.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Optimizations.hpp>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Vector2.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <unordered_map>

// clang-format off
namespace Game::Sprites { class SpriteSheet; }
// clang-format on

namespace Game::Sys
{

//! @brief Renders everything layered on top of the game world: HUD elements (meters, labels, icons, texts) and the shop and grimoire
//! overlays. Debug visualisations are handled by RenderDebugSystem.
class RenderOverlaySystem : public RenderSystem
{
public:
  //! @brief Construct a new Render Overlay System object. Loads the main and shop UI layout data from their JSON files.
  RenderOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
      : RenderSystem( reg, window, sound_bank )
  {
    m_main_ui_data = std::make_unique<Render::UiData>( "res/ui/ui.json" );
    m_shop_ui_data = std::make_unique<Render::UiData>( "res/ui/shop.json" );
  };

  //! @brief Entrypoint for rendering the UI overlays: screen-space particles, the shop and grimoire overlays, and the HUD.
  //! @param dt
  void render_overlay( sf::Time dt );

private:
  //! @brief Draw the screen-space particle sprites, i.e. the torch flame flickering over the UI inventory icon.
  void render_ui_particles();

  //! @brief Render the main UI's outline rectangles (e.g. panel borders).
  void render_ui_outlines();

  //! @brief Render the main UI's icons, excluding the inventory icon which is handled by render_ui_inventory_icon.
  void render_ui_icons();

  void render_ui_cadaver_icons();

  //! @brief Render the sprite in the Inventory UI.
  //! @note ParticleSprites are rendered seperately in render_ui_particles
  void render_ui_inventory_icon();

  //! @brief Render the main UI's meters (health, fear, infamy, despair, toxicity, inventory wear) and flash their outline colour on
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

  //! @brief Render the main UI's static text elements.
  void render_ui_texts();

  //! @brief Render the main UI's dynamic labels (cadaver count, wealth, current inventory item) and flash their colour on
  //! recent changes.
  //! @param dt
  void render_ui_labels( sf::Time dt );

  //! @brief Render the current dungeon level depth text while its display cooldown has not yet elapsed.
  void render_level_depth();

  //! @brief Render the shop UI overlay (outlines, item icons and slot labels) if a Cmp::Shop::Inventory is enabled in the registry.
  void render_shop_inventory_overlay();

  //! @brief Render the halo effect around the graves that are marked as ghost spawners. Only active when player is carrying ElderFlute.
  void render_ghost_grave_halos();

  //! @brief Render the countdown until the crypt maze next shuffles, if the shuffle timer has not already expired.
  //! @param pos
  //! @param size
  void render_crypt_maze_timer( sf::Vector2f pos, unsigned int size );

  template <typename FlashComponent>
  bool update_flash_toggle( sf::Time dt );

  //! @brief The main UI layout's current design-to-screen scale, for hard-coded HUD pixel values (outline widths, font sizes).
  [[nodiscard]] float ui_scale() const { return m_main_ui_data ? m_main_ui_data->scale() : 1.f; }

  //! @brief Scale a design-space (1920x1080) character size to the current UI scale.
  //! @param design_size character size in pixels at the layout's design resolution
  //! @return the scaled character size, at least 1
  [[nodiscard]] unsigned int scaled_char_size( unsigned int design_size ) const
  {
    return std::max( 1u, static_cast<unsigned int>( std::lround( static_cast<float>( design_size ) * ui_scale() ) ) );
  }

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

  //! @brief Draw a list of UI outline rectangles (panel borders).
  //! @param outlines
  void render_outlines( const std::vector<Render::UiData::Outline> &outlines )
  {
    sf::RectangleShape rect;
    for ( const auto &outline : outlines )
    {
      rect.setSize( outline.rect.size );
      rect.setPosition( outline.rect.position );
      rect.setFillColor( outline.fill_color );
      rect.setOutlineColor( outline.line_color );
      rect.setOutlineThickness( static_cast<float>( outline.line_thickness ) );
      rect.setRotation( outline.rotation );
      draw_screen( rect );
    }
  }

  //! @brief Layout data object for the main UI
  std::unique_ptr<Render::UiData> m_main_ui_data;

  //! @brief Layout data object for the shop scene overlay
  std::unique_ptr<Render::UiData> m_shop_ui_data;

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

#endif // SRC_SYSTEMS_RENDER_RENDEROVERLAYSYSTEM_HPP__
