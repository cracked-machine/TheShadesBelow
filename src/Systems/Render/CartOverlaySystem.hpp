#ifndef SRC_SYSTEMS_RENDER_CARTOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_CARTOVERLAYSYSTEM_HPP__

#include <Systems/Render/BaseOverlaySystem.hpp>
#include <Systems/Render/UiLayout.hpp>

#include <SFML/System/Time.hpp>

#include <memory>

namespace Game::Sys
{

//! @brief Draws a row, centred over the cart sprite, with one res/ui/barrel_slot.json slot per Cmp::Cart::Barrel while a
//! cart has Cmp::Cart::BarrelOverlayOpen. Every slot draws its part outlines, and the part sprites of its filled flags. Display
//! only: it takes no input and does not pause gameplay.
class CartOverlaySystem : public BaseOverlaySystem
{
public:
  //! @brief Construct a new Cart Overlay System object. Loads the slot and panel layouts from res/ui/barrel_slot.json
  //! and res/ui/barrel_panel.json.
  //! @param reg
  //! @param window
  //! @param sound_bank
  CartOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Draw the barrel slot row, if a cart's overlay is open.
  //! @param dt
  void update( sf::Time dt ) override;

private:
  //! @brief The layout of one barrel slot, moved to each slot's position as it is drawn.
  std::unique_ptr<Render::UiLayout> m_slot_ui_layout;

  //! @brief The border and background behind the row (res/ui/barrel_panel.json), authored around one slot. The padding
  //! between its size and the slot's is kept around the whole row.
  std::unique_ptr<Render::UiLayout> m_panel_ui_layout;

  //! @brief Name of the ui_outline in res/ui/barrel_panel.json that is resized to fit the row.
  static constexpr const char *kPanelOutlineName = "barrel_panel";
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_CARTOVERLAYSYSTEM_HPP__
