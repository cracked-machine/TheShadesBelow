#ifndef SRC_SYSTEMS_RENDER_OVERLAYARBITRATIONSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_OVERLAYARBITRATIONSYSTEM_HPP__

#include <Events/OverlayEvent.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/System/Time.hpp>
#include <entt/entity/fwd.hpp>

namespace Game::Sys
{

class Store;

//! @brief Decides which overlays (see BaseOverlaySystem) are open and which run each frame, and gives them one shared ImGui
//! frame.
//!
//! - Opens and closes the modal overlays (pause, grimoire) and toggles the F3 debug panels on Events::OverlayEvent,
//!   applying the modal policy (see can_open_modal()).
//! - Each gameplay frame, update() begins the ImGui frame, runs every overlay the current scene has, in draw order, and
//!   renders the ImGui frame. Running an ImGui frame every frame keeps ImGui's input queue from piling up, so overlays
//!   never have to flush it when they close.
class OverlayArbitrationSystem : public BaseSystem
{
public:
  //! @brief The modal overlays. While one is open, gameplay is paused and the other can't open.
  enum class Modal {
    //! @brief No modal overlay is open.
    NONE,
    //! @brief The pause overlay (Cmp::SceneSettings::Paused) is open.
    PAUSE,
    //! @brief The grimoire overlay (Cmp::Grimoire::is_enabled) is open.
    GRIMOIRE
  };

  //! @brief Construct a new Overlay Arbitration System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  //! @param sys The system store, to look up the overlay systems.
  OverlayArbitrationSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, Store &sys );

  //! @brief Run the current scene's overlays in draw order inside one shared ImGui frame: HUD, shop, level depth,
  //! tooltip, grimoire, pause and debug. Call from each gameplay scene's do_update() after RenderGameSystem::render_game().
  //! @param dt Real frame time, for ImGui and the overlays that run while paused.
  //! @param world_dt World frame time (zero while paused), for the HUD.
  void update( sf::Time dt, sf::Time world_dt );

  //! @brief The open modal overlay. The pause overlay wins over the grimoire.
  //! @param reg
  [[nodiscard]] static Modal active_modal( entt::registry &reg );

  //! @brief True if a modal overlay may open: no modal overlay is open and the F3 debug panels are hidden.
  //! @param reg
  [[nodiscard]] static bool can_open_modal( entt::registry &reg );

  //! @brief True if the F3 debug panels are shown: Cmp::SceneSettings::ShowDebugStats is on and no modal overlay is open.
  //! @param reg
  [[nodiscard]] static bool debug_panels_visible( entt::registry &reg );

  //! @brief True if the grimoire hover overlay may show: no modal overlay is open and the F3 debug panels are hidden.
  //! @param reg
  [[nodiscard]] static bool hover_allowed( entt::registry &reg );

private:
  //! @brief Open/close the modal overlays and toggle the debug panels, applying the modal policy. GRIMOIRE_PREV/NEXT are
  //! handled by GrimoireOverlaySystem.
  //! @param ev
  void on_overlay_event( const Events::OverlayEvent &ev );

  //! @brief Toggle the pause overlay. Opening is refused unless can_open_modal().
  void toggle_pause();

  //! @brief Toggle the grimoire overlay, debounced. Opening is refused unless can_open_modal() and the scene has a grimoire.
  void toggle_grimoire();

  //! @brief Toggle the F3 debug panels (Cmp::SceneSettings::ShowDebugStats). Refused while a modal overlay is open.
  void toggle_debug();

  //! @brief Begin the shared ImGui frame.
  //! @param dt
  void begin_frame( sf::Time dt );

  //! @brief Render the shared ImGui frame on top of everything drawn so far.
  void end_frame();

  //! @brief True while the scene is paused (Cmp::SceneSettings::Paused).
  [[nodiscard]] bool is_paused();

  //! @brief True if the current scene has the shop overlay.
  [[nodiscard]] bool has_shop();

  //! @brief True if the current scene has the grimoire overlay (open book and hover).
  [[nodiscard]] bool has_grimoire();

  //! @brief True if the current scene shows the level depth banner. Only GraveyardScene advances its display timer.
  [[nodiscard]] bool has_level_depth();

  //! @brief The system store, to look up the overlay systems. They all exist once the store is constructed.
  Store &m_sys;

  //! @brief Time since the grimoire was last toggled, used to debounce rapid toggling.
  sf::Time m_grimoire_toggle_debounce;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_OVERLAYARBITRATIONSYSTEM_HPP__
