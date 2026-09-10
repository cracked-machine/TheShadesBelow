#include <Factory/MultiblockFactoryImpl.hpp>

namespace Game::Factory::Multiblock
{

namespace detail
{

// clang-format off
template void create_multiblock<Cmp::Crypt::BuildingMultiBlock>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, size_t );
template void create_multiblock<Cmp::Crypt::ObjectiveMultiBlock>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, size_t );

template void update_segments<Cmp::Crypt::BuildingMultiBlock, Cmp::Crypt::BuildingSegment>( entt::registry &, const Sprites::SpriteSheet &, entt::entity, Cmp::Crypt::BuildingMultiBlock );
template void update_segments<Cmp::Crypt::ObjectiveMultiBlock, Cmp::Crypt::ObjectiveSegment>( entt::registry &, const Sprites::SpriteSheet &, entt::entity, Cmp::Crypt::ObjectiveMultiBlock );

template std::vector<entt::entity> create_multiblock_segments<Cmp::Crypt::BuildingMultiBlock, Cmp::Crypt::BuildingSegment>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, PathFinding::SpatialHashGrid * );
template std::vector<entt::entity> create_multiblock_segments<Cmp::Crypt::ObjectiveMultiBlock, Cmp::Crypt::ObjectiveSegment>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, PathFinding::SpatialHashGrid * );
// clang-format on

} // namespace detail

// clang-format off
template std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments<Cmp::Crypt::BuildingMultiBlock, Cmp::Crypt::BuildingSegment>( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet &, size_t, float, PathFinding::SpatialHashGrid * );
template std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments<Cmp::Crypt::ObjectiveMultiBlock, Cmp::Crypt::ObjectiveSegment>( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet &, size_t, float, PathFinding::SpatialHashGrid * );
// clang-format on

} // namespace Game::Factory::Multiblock
