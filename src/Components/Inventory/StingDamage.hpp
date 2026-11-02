#ifndef SRC_COMPONENTS_INVENTORY_STINGDAMAGE_HPP__
#define SRC_COMPONENTS_INVENTORY_STINGDAMAGE_HPP__

namespace Game::Cmp::Inventory
{

//! @brief Tag on a world item or player inventory slot whose spawn_action or carry_action damages the player (negative
//! "health" in items.json, e.g. nettleleaf, hawthornflower, burr). Added when the item is created - except when it is
//! foraged with gloves - and carried with the item between the world and the player inventory. Without it the item's
//! spawn_action and carry_action still apply, but deal no damage. See Cmp::WorldItem::stings().
struct StingDamage
{
};

} // namespace Game::Cmp::Inventory

#endif // SRC_COMPONENTS_INVENTORY_STINGDAMAGE_HPP__
