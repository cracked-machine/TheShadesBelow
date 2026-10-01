#ifndef SRC_COMPONENTS_LOOT_HPP__
#define SRC_COMPONENTS_LOOT_HPP__

#include <Sprites/SpriteSheet.hpp>

namespace Game::Cmp
{

//! @brief Mark an entity as a dropped loot item
struct Loot
{
  //! @brief Whether this loot item is currently active/available.
  [[maybe_unused]] bool active = true;
};

} // namespace Game::Cmp
#endif // SRC_COMPONENTS_LOOT_HPP__
