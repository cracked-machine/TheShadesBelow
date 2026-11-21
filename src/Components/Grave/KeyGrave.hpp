#ifndef SRC_CMPS_GRAVE_KEYGRAVE_HPP__
#define SRC_CMPS_GRAVE_KEYGRAVE_HPP__

namespace Game::Cmp::Grave
{

//! @brief Tags the un-exhumed grave that holds the exit key. Given to one random grave by GraveSystem when an altar is
//! activated; a guide crow (Cmp::Npc::Crow::guide_grave) leads the player to it. Exhuming it drops the exit key instead
//! of the grave's own consequence, and the tag is removed.
struct KeyGrave
{
};

} // namespace Game::Cmp::Grave

#endif // SRC_CMPS_GRAVE_KEYGRAVE_HPP__
