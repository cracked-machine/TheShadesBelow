#ifndef SRC_COMPONENTS_PATHTILE_HPP__
#define SRC_COMPONENTS_PATHTILE_HPP__

namespace Game::Cmp
{

//! @brief Tags a path tile: flat, walkable ground drawn from the map's path layer. Path tiles are in Scene::m_reserved_sm
//! only so procgen won't repurpose them; things can still be spawned or dropped on them (see Utils::is_free_spawn_area()).
struct PathTile
{
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_PATHTILE_HPP__
