#include <Components/Cart/MultiBlock.hpp>
#include <Components/Cart/Segment.hpp>
#include <Factory/MultiblockFactoryImpl.hpp>

namespace Game::Factory::Multiblock
{

// clang-format off
template std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments<Cmp::Cart::MultiBlock, Cmp::Cart::Segment>( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet &, size_t, float, PathFinding::SpatialHashGrid *, const PathFinding::SpatialHashGrid * );
// clang-format on

} // namespace Game::Factory::Multiblock
