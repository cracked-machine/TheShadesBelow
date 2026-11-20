#ifndef SRC_COMPONENTS_WEAPONDAMAGE_HPP__
#define SRC_COMPONENTS_WEAPONDAMAGE_HPP__

#include <Systems/Stores/StoreKey.hpp>

namespace Game::Cmp
{

//! @brief Damage per weapon hit, as read from a "damage_to_weapon" or "damage_from_weapon" object in items.json/npc.json:
//! { "pickaxe": int, "axe": int, "shovel": int, "bow": int }.
struct WeaponDamage
{
  int pickaxe{ 0 };
  int axe{ 0 };
  int shovel{ 0 };
  int bow{ 0 };

  //! @brief The damage for the weapon item, e.g. "item.shovel". Zero for an item that isn't a weapon.
  //! @param weapon The weapon's item key.
  [[nodiscard]] int get( const Sys::ItemKey &weapon ) const
  {
    if ( weapon == "item.pickaxe" ) return pickaxe;
    if ( weapon == "item.axe" ) return axe;
    if ( weapon == "item.shovel" ) return shovel;
    if ( weapon == "item.longbow" ) return bow;
    return 0;
  }
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_WEAPONDAMAGE_HPP__
