#include <Components/Horse/MultiBlock.hpp>
#include <Components/Horse/Segment.hpp>
#include <Factory/MultiblockFactoryImpl.hpp>

namespace Game::Factory::Multiblock
{

// clang-format off
template std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments<Cmp::Horse::MultiBlock, Cmp::Horse::Segment>( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet &, size_t, float, PathFinding::SpatialHashGrid *, const PathFinding::SpatialHashGrid * );
// clang-format on

} // namespace Game::Factory::Multiblock
