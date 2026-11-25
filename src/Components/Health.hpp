#ifndef SRC_COMPONENTS_HEALTH_HPP__
#define SRC_COMPONENTS_HEALTH_HPP__

namespace Game::Cmp
{

//! @brief The health of a plant or NPC, as a percentage (0-100). Weapon hits lower it by the target's
//! "damage_from_weapon" (items.json/npc.json), see Events::WeaponHitEvent. Plants also lose health to decay,
//! foraging, kill_npc wear and damage_npc through Events::UpdateDmgEvent. NPCs also lose health to damage_npc plants.
class Health
{
public:
  //! @brief Construct a new Health object.
  //! @param level Initial health, as a percentage (0-100).
  explicit Health( float level = 100.f )
      : m_level( level )
  {
  }

  //! @brief The remaining health, as a percentage (0-100).
  float m_level;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_HEALTH_HPP__
