#ifndef SRC_COMPONENTS_NPC_AZAZEL_HPP__
#define SRC_COMPONENTS_NPC_AZAZEL_HPP__

#include <SFML/System/Time.hpp>

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Azazel NPC (a hostile NPC type). Azazel shoots fireballs at the
//! player and despawns once all of its Cmp::Npc::SummonOffering items have been sacrificed (see NpcSystem::update_azazel()).
struct Azazel
{
  sf::Time sfx_cooldown{ sf::Time::Zero };
  //! @brief Time since Azazel last shot a fireball, see Cmp::Persist::AzazelFireballCooldown.
  sf::Time fire_cooldown{ sf::Time::Zero };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_AZAZEL_HPP__
