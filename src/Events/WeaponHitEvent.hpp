#ifndef SRC_EVENTS_WEAPONHITEVENT_HPP__
#define SRC_EVENTS_WEAPONHITEVENT_HPP__

#include <SFML/System/Vector2.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <entt/entity/entity.hpp>

namespace Game::Events
{

//! @brief A weapon hit a plant, an NPC or the player. Fire and forget: the system that owns the target applies the hit.
//! @details PlantSystem handles Cmp::PlantMultiBlock targets and NpcSystem Cmp::Npc::NPC targets: each takes the target's
//! "damage_from_weapon" (items.json/npc.json) from its Cmp::Health, wears the held weapon by the target's
//! "damage_to_weapon", and kills the target at zero health. PlayerSystem handles the player, applying the attacking
//! NPC's projectile_action (npc.json).
class WeaponHitEvent
{
public:
  //! @param target The plant, NPC or player entity that was hit.
  //! @param weapon The weapon's key: "item.shovel", "item.longbow" or Cmp::WeaponDamage::kGunKey.
  //! @param direction The direction of the blow, from the attacker towards the target. Need not be normalized. A hit
  //! that doesn't kill an NPC knocks it back this way.
  //! @param source_npc The NPC that made the attack, e.g. "npc.nightwatchman". Empty for the player's weapons.
  WeaponHitEvent( entt::entity target, Sys::ItemKey weapon, sf::Vector2f direction, Sys::NpcKey source_npc = {} )
      : m_target( target ),
        m_weapon( std::move( weapon ) ),
        m_direction( direction ),
        m_source_npc( std::move( source_npc ) )
  {
  }

  entt::entity m_target;
  Sys::ItemKey m_weapon;
  sf::Vector2f m_direction;
  Sys::NpcKey m_source_npc;
};

} // namespace Game::Events

#endif // SRC_EVENTS_WEAPONHITEVENT_HPP__
