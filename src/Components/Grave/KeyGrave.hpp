#ifndef SRC_CMPS_GRAVE_KEYGRAVE_HPP__
#define SRC_CMPS_GRAVE_KEYGRAVE_HPP__

namespace Game::Cmp::Grave
{

//! @brief Tags an un-exhumed grave that holds a crypt key. LevelGenerator gives it to one random grave in each map
//! quadrant. Exhuming it drops a crypt key instead of the grave's own consequence, and the tag is removed.
struct KeyGrave
{
};

} // namespace Game::Cmp::Grave

#endif // SRC_CMPS_GRAVE_KEYGRAVE_HPP__
