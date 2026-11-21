#ifndef SRC_CMPS_PLANT_GRASS_HPP__
#define SRC_CMPS_PLANT_GRASS_HPP__

#include <string_view>

namespace Game::Cmp::Plant
{

//! @brief items.json key of the grass plant, placed by Sys::LevelGenerator::gen_grass().
inline constexpr std::string_view kGrassItem = "item.plant.grass";

//! @brief Tags a grass Cmp::PlantMultiBlock root. Grass is passable (see Utils::get_grass_speed_multiplier()) and, unlike
//! other plants, doesn't reserve its tile in reserved_sm, so whatever later takes over the cell (a hazard, a replanted
//! plant) removes it instead, see Factory::Plant::destroy_grass_in().
struct Grass
{
};

} // namespace Game::Cmp::Plant

#endif // SRC_CMPS_PLANT_GRASS_HPP__
