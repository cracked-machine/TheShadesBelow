#ifndef SRC_COMPONENTS_WORLDTILE_HPP__
#define SRC_COMPONENTS_WORLDTILE_HPP__

namespace Game::Cmp
{

//! @brief Tag for a floor tile created from level data (one per map cell), see Factory::Obstacle::create_world_pos().
//! @note Use this, not Cmp::Armable, to mean "world tile". Obstacles, loot/NPC containers and hazard cells are
//! components added to these same entities; multiblock segments, NPCs and dropped items are separate entities.
struct WorldTile
{
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_WORLDTILE_HPP__
