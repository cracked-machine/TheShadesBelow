#ifndef SRC_COMPONENTS_NPC_SUMMONOFFERING_HPP__
#define SRC_COMPONENTS_NPC_SUMMONOFFERING_HPP__

#include <Components/UUID.hpp>

namespace Game::Cmp::Npc
{

//! @brief Marks an item (world item or inventory slot) dropped by the sacrifice that summoned an NPC, e.g. the jewelry
//! dropped alongside Azazel. Azazel stays until all of its offerings have been sacrificed.
struct SummonOffering
{
  //! @brief The Cmp::UUID of the altar the summoning sacrifice was made at, i.e. the summoned NPC's Cmp::Npc::SpawnedBy.
  Cmp::UUID m_summoner;
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_SUMMONOFFERING_HPP__
