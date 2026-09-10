#include <Factory/MultiblockFactoryImpl.hpp>

namespace Game::Factory::Multiblock
{

namespace detail
{

// clang-format off
template void create_multiblock<Cmp::Altar::MultiBlock>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, size_t );
template void create_multiblock<Cmp::Grave::MultiBlock>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, size_t );
template void create_multiblock<Cmp::Grave::ExitMultiBlock>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, size_t );

template void update_segments<Cmp::Altar::MultiBlock, Cmp::Altar::Segment>( entt::registry &, const Sprites::SpriteSheet &, entt::entity, Cmp::Altar::MultiBlock );
template void update_segments<Cmp::Grave::MultiBlock, Cmp::Grave::Segment>( entt::registry &, const Sprites::SpriteSheet &, entt::entity, Cmp::Grave::MultiBlock );
template void update_segments<Cmp::Grave::ExitMultiBlock, Cmp::Grave::ExitSegment>( entt::registry &, const Sprites::SpriteSheet &, entt::entity, Cmp::Grave::ExitMultiBlock );

template std::vector<entt::entity> create_multiblock_segments<Cmp::Altar::MultiBlock, Cmp::Altar::Segment>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, PathFinding::SpatialHashGrid * );
template std::vector<entt::entity> create_multiblock_segments<Cmp::Grave::MultiBlock, Cmp::Grave::Segment>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, PathFinding::SpatialHashGrid * );
template std::vector<entt::entity> create_multiblock_segments<Cmp::Grave::ExitMultiBlock, Cmp::Grave::ExitSegment>( entt::registry &, entt::entity, const Cmp::UUID &, Cmp::Position, const Sprites::SpriteSheet &, PathFinding::SpatialHashGrid * );
// clang-format on

} // namespace detail

// clang-format off
template std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments<Cmp::Altar::MultiBlock, Cmp::Altar::Segment>( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet &, size_t, float, PathFinding::SpatialHashGrid * );
template std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments<Cmp::Grave::MultiBlock, Cmp::Grave::Segment>( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet &, size_t, float, PathFinding::SpatialHashGrid * );
template std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments<Cmp::Grave::ExitMultiBlock, Cmp::Grave::ExitSegment>( entt::registry &, sf::Vector2f, const Sprites::SpriteSheet &, size_t, float, PathFinding::SpatialHashGrid * );
// clang-format on

} // namespace Game::Factory::Multiblock
