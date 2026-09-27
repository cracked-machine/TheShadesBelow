#ifndef SRC_FACTORY_ACTIONFACTORY_HPP__
#define SRC_FACTORY_ACTIONFACTORY_HPP__

namespace Game::Factory::Action
{

//! @brief Add a Cmp::Player::EatingTimeAccumulator to the player if they have ".forage" inventory item
//! @note Processed by InventorySystem::update()
//! @param reg
void try_eat_inventory( entt::registry &reg );

//! @brief Add a Cmp::Plant::BurningTimeAccumulator to a nearby plant.
//! @note  Processed by ActionSystem::update()
//! @param reg
void try_burn_worlditem( entt::registry &reg );

} // namespace Game::Factory::Action

#endif // SRC_FACTORY_ACTIONFACTORY_HPP__