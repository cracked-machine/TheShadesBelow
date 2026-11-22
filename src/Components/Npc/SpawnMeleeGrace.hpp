#ifndef SRC_COMPONENTS_NPC_SPAWNMELEEGRACE_HPP__
#define SRC_COMPONENTS_NPC_SPAWNMELEEGRACE_HPP__

namespace Game::Cmp::Npc
{

//! @brief Tags an NPC spawned centred on its source (Cmp::SpawnNpcEffect::Location::CENTER), e.g. a tick from grass
//! killed by the shovel. It lands under the mouse, so the held attack that spawned it would hit it on the next frame.
//! Sys::ActionSystem melee ignores it until the left mouse button is released, then removes the tag.
struct SpawnMeleeGrace
{
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_SPAWNMELEEGRACE_HPP__
