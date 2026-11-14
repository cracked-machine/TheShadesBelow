#ifndef SRC_SYSTEMS_RENDER_SHOPOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_SHOPOVERLAYSYSTEM_HPP__

#include <Systems/Render/BaseOverlaySystem.hpp>
#include <Systems/Render/UiData.hpp>

#include <SFML/System/Time.hpp>

#include <memory>

namespace Game::Sys
{

//! @brief Draws the shop overlay (outlines, item icons, torch flames and slot labels from res/ui/shop.json) while a
//! Cmp::Shop::Inventory is enabled, i.e. while the player stands at the shopkeeper.
class ShopOverlaySystem : public BaseOverlaySystem
{
public:
  //! @brief Construct a new Shop Overlay System object. Loads the shop layout from res/ui/shop.json.
  //! @param reg
  //! @param window
  //! @param sound_bank
  ShopOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Draw the shop overlay if a Cmp::Shop::Inventory is enabled in the registry. No-op otherwise.
  //! @param dt
  void update( sf::Time dt ) override;

private:
  //! @brief Layout data object for the shop overlay
  std::unique_ptr<Render::UiData> m_shop_ui_data;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_SHOPOVERLAYSYSTEM_HPP__
