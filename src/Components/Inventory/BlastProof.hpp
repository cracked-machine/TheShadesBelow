#ifndef SRC_COMPONENTS_INVENTORY_BLASTPROOF_HPP__
#define SRC_COMPONENTS_INVENTORY_BLASTPROOF_HPP__

namespace Game::Cmp::Inventory
{

//! @brief Tags a world item that the current explosion sequence must not destroy, e.g. loot dropped by a grave the
//! blast exhumed. Sys::BombSystem clears every tag once no armed bombs remain.
struct BlastProof
{
};

} // namespace Game::Cmp::Inventory

#endif // SRC_COMPONENTS_INVENTORY_BLASTPROOF_HPP__
