#ifndef SRC_COMPONENTS_DAMAGEPOPUP_HPP__
#define SRC_COMPONENTS_DAMAGEPOPUP_HPP__

#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

namespace Game::Cmp
{

//! @brief A floating damage number, shown when a weapon hit damages a plant or NPC (Cmp::Persist::ShowDamageText). Lives on
//! its own entity, so it outlives a target killed by the hit. It has no Cmp::Position, so systems scanning positions
//! never see it. Drawn, aged and destroyed by HudOverlaySystem::render_damage_popups().
struct DamagePopup
{
  //! @brief World position the number rises from: the top-centre of the target when it was hit.
  sf::Vector2f world_pos;

  //! @brief The health points taken by the hit.
  float amount{ 0.f };

  //! @brief Time since the hit.
  sf::Time elapsed{ sf::Time::Zero };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_DAMAGEPOPUP_HPP__
