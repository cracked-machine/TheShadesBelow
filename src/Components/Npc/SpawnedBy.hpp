#ifndef SRC_COMPONENTS_NPC_SPAWNEDBY_HPP__
#define SRC_COMPONENTS_NPC_SPAWNEDBY_HPP__

#include <Components/UUID.hpp>

namespace Game::Cmp::Npc
{

//! @brief Links an NPC to the world item whose action side effect spawned it (see Cmp::SpawnNpcEffect::max_instances).
struct SpawnedBy
{
  //! @brief The Cmp::UUID of the spawning world item.
  Cmp::UUID m_source;
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_SPAWNEDBY_HPP__
