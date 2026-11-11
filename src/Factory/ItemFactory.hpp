#ifndef SRC_FACTORY_ITEMFACTORY_HPP__
#define SRC_FACTORY_ITEMFACTORY_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <cstddef>
#include <entt/entity/fwd.hpp>

namespace Game::Factory::Item
{

//! @brief Add a Cmp::Grave::CurseTablet to a newly created curse tablet item.
//! @param item_entt
//! @param item Item identifier; its name selects the curse type (ghost/skeleton/adder).
//! @param sprite_idx The sprite variant used; hits required to destroy it is `sprite_idx + 1`.
void add_curse_tablet( entt::registry &reg, entt::entity item_entt, const Sys::ItemKey &item, size_t sprite_idx );

} // namespace Game::Factory::Item

#endif // SRC_FACTORY_ITEMFACTORY_HPP__