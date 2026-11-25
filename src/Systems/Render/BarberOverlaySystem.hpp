#ifndef SRC_SYSTEMS_RENDER_BARBEROVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_BARBEROVERLAYSYSTEM_HPP__

#include <Systems/Render/BaseOverlaySystem.hpp>

#include <SFML/System/Time.hpp>

#include <cstddef>

// clang-format off
namespace Game::Events { struct OverlayEvent; }
// clang-format on

namespace Game::Sys
{

class Store;

//! @brief The barber surgeon's heal menu: while the player stands at the surgeon in the BarberSurgeonScene, lists the
//! stats it can heal (see ShopSystem::HealStat). The mouse wheel moves the selection and E buys the selected heal with
//! gold (ShopSystem::buy_heal), after which the player leaves the scene after a short delay (HealingSpringSystem::request_delayed_exit).
class BarberOverlaySystem : public BaseOverlaySystem
{
public:
  //! @brief Construct a new Barber Overlay System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  //! @param sys The system store, to look up HealingSpringSystem and ShopSystem.
  BarberOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, Store &sys );

  //! @brief Draw the heal menu while the player is at the surgeon. No-op otherwise.
  //! @param dt
  void update( sf::Time dt ) override;

private:
  //! @brief Handle the HEAL_PREV/HEAL_NEXT/HEAL_PURCHASE overlay actions while the menu is active.
  //! @param ev
  void on_overlay_event( const Events::OverlayEvent &ev );

  //! @brief True if the menu is shown and accepts input: the barber surgeon scene is current, the player is at the
  //! surgeon, no modal overlay is open and no exit is pending.
  [[nodiscard]] bool is_active();

  //! @brief The system store, to look up HealingSpringSystem and ShopSystem.
  Store &m_sys;

  //! @brief The selected row (a ShopSystem::HealStat).
  std::size_t m_selected{ 0 };
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_BARBEROVERLAYSYSTEM_HPP__
