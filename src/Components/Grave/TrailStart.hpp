#ifndef SRC_CMPS_GRAVE_TRAILSTART_HPP__
#define SRC_CMPS_GRAVE_TRAILSTART_HPP__

namespace Game::Cmp::Grave
{

//! @brief Tags an un-exhumed grave as a possible start of the key grave trail. Given to several graves by GraveSystem
//! when an altar is activated; exhuming any one starts the trail and the tag is removed from all the others.
//! TrailGuideSystem guides the player from the activated altar to each tagged grave.
struct TrailStart
{
};

} // namespace Game::Cmp::Grave

#endif // SRC_CMPS_GRAVE_TRAILSTART_HPP__
