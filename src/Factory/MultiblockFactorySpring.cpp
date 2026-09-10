#include <Factory/MultiblockFactoryImpl.hpp>

namespace Game::Factory::Multiblock
{

namespace detail
{

// clang-format off
template void create_multiblock<Cmp::HealingSpringMultiBlock>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, size_t );
template void create_multiblock<Cmp::HealingSpringBuildingMultiBlock>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, size_t );

template void update_segments<Cmp::HealingSpringMultiBlock, Cmp::HealingSpringSegment>( entt::registry &, const Sprites::SpriteSheet &, entt::entity, Cmp::HealingSpringMultiBlock );
template void update_segments<Cmp::HealingSpringBuildingMultiBlock, Cmp::HealingSpringBuildingSegment>( entt::registry &, const Sprites::SpriteSheet &, entt::entity, Cmp::HealingSpringBuildingMultiBlock );

template std::vector<entt::entity> create_multiblock_segments<Cmp::HealingSpringMultiBlock, Cmp::HealingSpringSegment>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, PathFinding::SpatialHashGrid * );
template std::vector<entt::entity> create_multiblock_segments<Cmp::HealingSpringBuildingMultiBlock, Cmp::HealingSpringBuildingSegment>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, PathFinding::SpatialHashGrid * );
// clang-format on

} // namespace detail

// clang-format off
template std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments<Cmp::HealingSpringMultiBlock, Cmp::HealingSpringSegment>( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet &, size_t, float, PathFinding::SpatialHashGrid * );
template std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments<Cmp::HealingSpringBuildingMultiBlock, Cmp::HealingSpringBuildingSegment>( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet &, size_t, float, PathFinding::SpatialHashGrid * );
// clang-format on

} // namespace Game::Factory::Multiblock
