#include <Factory/MultiblockFactoryImpl.hpp>

namespace Game::Factory::Multiblock
{

namespace detail
{

// clang-format off
template void create_multiblock<Cmp::Ruin::StairsLowerMultiBlock>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, size_t );
template void create_multiblock<Cmp::Ruin::StairsUpperMultiBlock>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, size_t );
template void create_multiblock<Cmp::Ruin::StairsBalustradeMultiBlock>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, size_t );
template void create_multiblock<Cmp::Ruin::StairsGateMultiBlock>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, size_t );

template void update_segments<Cmp::Ruin::StairsLowerMultiBlock, Cmp::Ruin::StairsSegment>( entt::registry &, const Sprites::SpriteSheet &, entt::entity, Cmp::Ruin::StairsLowerMultiBlock );
template void update_segments<Cmp::Ruin::StairsUpperMultiBlock, Cmp::Ruin::StairsSegment>( entt::registry &, const Sprites::SpriteSheet &, entt::entity, Cmp::Ruin::StairsUpperMultiBlock );
template void update_segments<Cmp::Ruin::StairsBalustradeMultiBlock, Cmp::Ruin::StairsSegment>( entt::registry &, const Sprites::SpriteSheet &, entt::entity, Cmp::Ruin::StairsBalustradeMultiBlock );
template void update_segments<Cmp::Ruin::StairsGateMultiBlock, Cmp::Ruin::GateSegment>( entt::registry &, const Sprites::SpriteSheet &, entt::entity, Cmp::Ruin::StairsGateMultiBlock );

template std::vector<entt::entity> create_multiblock_segments<Cmp::Ruin::StairsLowerMultiBlock, Cmp::Ruin::StairsSegment>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, PathFinding::SpatialHashGrid * );
template std::vector<entt::entity> create_multiblock_segments<Cmp::Ruin::StairsUpperMultiBlock, Cmp::Ruin::StairsSegment>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, PathFinding::SpatialHashGrid * );
template std::vector<entt::entity> create_multiblock_segments<Cmp::Ruin::StairsBalustradeMultiBlock, Cmp::Ruin::StairsSegment>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, PathFinding::SpatialHashGrid * );
template std::vector<entt::entity> create_multiblock_segments<Cmp::Ruin::StairsGateMultiBlock, Cmp::Ruin::GateSegment>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, PathFinding::SpatialHashGrid * );
// clang-format on

} // namespace detail

// clang-format off
template std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments<Cmp::Ruin::StairsLowerMultiBlock, Cmp::Ruin::StairsSegment>( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet &, size_t, float, PathFinding::SpatialHashGrid * );
template std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments<Cmp::Ruin::StairsUpperMultiBlock, Cmp::Ruin::StairsSegment>( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet &, size_t, float, PathFinding::SpatialHashGrid * );
template std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments<Cmp::Ruin::StairsBalustradeMultiBlock, Cmp::Ruin::StairsSegment>( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet &, size_t, float, PathFinding::SpatialHashGrid * );
template std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments<Cmp::Ruin::StairsGateMultiBlock, Cmp::Ruin::GateSegment>( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet &, size_t, float, PathFinding::SpatialHashGrid * );
// clang-format on

} // namespace Game::Factory::Multiblock
