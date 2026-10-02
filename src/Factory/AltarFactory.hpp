#ifndef SRC_FACTORY_ALTARFACTORY_HPP__
#define SRC_FACTORY_ALTARFACTORY_HPP__

#include <Components/Position.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <entt/entity/fwd.hpp>

namespace Game::Factory::Altar
{

//! @brief Spawn the one-shot sacrifice animation played on an altar.
//! @param reg
//! @param pos Position and size of the animation entity.
//! @param anim_type Sprite type to play.
void create_altar_sacrifice_anim( entt::registry &reg, Cmp::Position pos, Sys::SpriteKey anim_type );

} // namespace Game::Factory::Altar

#endif // SRC_FACTORY_ALTARFACTORY_HPP__
