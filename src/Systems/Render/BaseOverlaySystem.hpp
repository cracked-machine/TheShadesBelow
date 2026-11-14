#ifndef SRC_SYSTEMS_RENDER_BASEOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_BASEOVERLAYSYSTEM_HPP__

#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Render/UiData.hpp>

#include <SFML/System/Time.hpp>

namespace Game::Sys
{

//! @brief Base class for the overlays drawn on top of the game world: HudOverlaySystem, ShopOverlaySystem,
//! PauseOverlaySystem, GrimoireOverlaySystem, DebugOverlaySystem and TooltipOverlaySystem.
//!
//! Each overlay does all of its per-frame work (timers, state, SFML drawing and ImGui windows) in update(). Overlays only
//! add ImGui windows: the ImGui frame itself is begun and rendered once per frame by OverlayArbitrationSystem, which also
//! decides which overlays run and which modal overlay may open.
class BaseOverlaySystem : public RenderSystem
{
public:
  //! @brief polymorphic destructor for derived classes
  ~BaseOverlaySystem() override = default;

  //! @brief Do the overlay's per-frame work: advance its timers and state, and draw it. Called by OverlayArbitrationSystem
  //! between the begin and end of the shared ImGui frame.
  //! @param dt
  virtual void update( sf::Time dt ) = 0;

protected:
  //! @brief Construct a new Base Overlay System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  BaseOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Draw a layout's outline rectangles (panel borders) in screen view coordinates.
  //! @param ui_data
  void render_outlines( const Render::UiData &ui_data );

  //! @brief The layout's current design-to-screen scale, for hard-coded pixel values (outline widths, font sizes).
  //! @param ui_data The layout, or nullptr.
  //! @return The scale, or 1 if `ui_data` is nullptr.
  [[nodiscard]] static float ui_scale( const Render::UiData *ui_data ) { return ui_data ? ui_data->scale() : 1.f; }

  //! @brief Scale a design-space character size to the layout's current UI scale.
  //! @param design_size character size in pixels at the layout's design resolution
  //! @param ui_data The layout, or nullptr for no scaling.
  //! @return the scaled character size, at least 1
  [[nodiscard]] static unsigned int scaled_char_size( unsigned int design_size, const Render::UiData *ui_data );

  //! @brief Side effects of opening a modal overlay (pause, grimoire): cancel any bow draw, then pause the sound effects.
  void on_modal_open();

  //! @brief Side effects of closing a modal overlay: resume the sound effects.
  void on_modal_close();
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_BASEOVERLAYSYSTEM_HPP__
