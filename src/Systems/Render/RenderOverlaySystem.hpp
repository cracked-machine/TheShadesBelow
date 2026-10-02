#ifndef SRC_SYSTEMS_RENDER_RENDEROVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_RENDEROVERLAYSYSTEM_HPP__

#include <Components/Position.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Render/UiData.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Optimizations.hpp>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>

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

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Draw the screen-space particle sprites, i.e. the candle flame flickering over the UI inventory icon.
  void render_ui_particles();

  //! @brief Render the main UI's outline rectangles (e.g. panel borders).
  void render_ui_outlines();

  //! @brief Render the main UI's icons, excluding the inventory icon which is handled by render_ui_inventory_icon.
  void render_ui_icons();

  //! @brief Render the sprite in the Inventory UI.
  //! @note ParticleSprites are rendered seperately in render_ui_particles
  void render_ui_inventory_icon();

  //! @brief Render the main UI's meters (health, fear, infamy, despair, toxicity, inventory wear) and flash their outline colour on
  //! recent changes.
  //! @param dt
  void render_ui_meters( sf::Time dt );

  //! @brief Render the main UI's static text elements.
  void render_ui_texts();

  //! @brief Render the main UI's dynamic labels (blast radius, cadaver count, wealth, current inventory item) and flash their colour on
  //! recent changes.
  //! @param dt
  void render_ui_labels( sf::Time dt );

  //! @brief Render the current dungeon level depth text while its display cooldown has not yet elapsed.
  void render_level_depth();

  //! @brief Render the shop UI overlay (outlines, item icons and slot labels) if a Cmp::Shop::Inventory is enabled in the registry.
  void render_shop_inventory_overlay();

  //! @brief Render the grimoire UI overlay listing spell entries and their shown/hidden state, if a Cmp::Grimoire is enabled in the
  //! registry.
  void render_grimoire_inventory_overlay();

  //! @brief Render the countdown until the crypt maze next shuffles, if the shuffle timer has not already expired.
  //! @param pos
  //! @param size
  void render_crypt_maze_timer( sf::Vector2f pos, unsigned int size );

  template <typename FlashComponent>
  bool update_flash_toggle( sf::Time dt );

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
      draw_screen( rect );
    }
  }

  //! @brief Layout data object for the main UI
  std::unique_ptr<Render::UiData> m_main_ui_data;

  //! @brief Layout data object for the shop scene overlay
  std::unique_ptr<Render::UiData> m_shop_ui_data;

  //! @brief Screen flash frequency
  int m_ui_flash_factor{ 300 };
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_RENDEROVERLAYSYSTEM_HPP__
