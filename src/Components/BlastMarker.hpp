#ifndef SRC_COMPONENTS_BLASTMARKER_HPP__
#define SRC_COMPONENTS_BLASTMARKER_HPP__

namespace Game::Cmp
{

//! @brief Tags a visual-only Cmp::Armed tile: a cell inside a blast radius that must not be detonated (reserved
//! positions such as graves, world items or the exit). It flashes with the rest of the blast pattern in the
//! Cmp::Persist::ReservedArmedBlockColourBorder/Fill colours, drawn in the z-order queue just above the cell's sprites. Sys::BombSystem
//! destroys it, without any blast effects, once its fuse expires. The entity holds only Cmp::Position, Cmp::Armed,
//! Cmp::ZOrderValue and this tag.
struct BlastMarker
{
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_BLASTMARKER_HPP__
