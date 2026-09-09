#include <Components/Armable.hpp>
#include <Components/LootContainer.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Position.hpp>
#include <Factory/LootFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Utils/Random.hpp>

namespace Game::Factory::Loot
{

void create_loot_container( entt::registry &reg, entt::entity entt, Cmp::Position pos_cmp, Sprites::SpriteMetaType sprite_type,
                            std::size_t sprite_tile_idx, float zorder )
{

  reg.emplace_or_replace<Cmp::Armable>( entt );
  reg.emplace_or_replace<Cmp::LootContainer>( entt );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( entt, Cmp::AnimData::Config{ 
        .sprite_type = sprite_type, 
        .frame_index_offset = sprite_tile_idx,
        .framerate = 0.2,
        .enabled = false,
        .anim_type =   Cmp::AnimType::ONESHOTRESET
  });
  // clang-format on
  reg.emplace_or_replace<Cmp::ZOrderValue>( entt, pos_cmp.position.y - zorder );
}

void destroy_loot_container( entt::registry &registry, entt::entity loot_container_entity,
                             const PathFinding::SpatialHashGridSharedPtr &reserved_sm )
{
  registry.remove<Cmp::LootContainer>( loot_container_entity );
  if ( reserved_sm )
  {
    if ( auto *pos_cmp = registry.try_get<Cmp::Position>( loot_container_entity ) ) reserved_sm->remove( loot_container_entity, *pos_cmp );
  }
  registry.remove<Cmp::AnimData>( loot_container_entity );
  registry.remove<Cmp::ZOrderValue>( loot_container_entity );
}

void destroy_loot_drop( entt::registry &registry, entt::entity loot_entity ) { registry.destroy( loot_entity ); }

} // namespace Game::Factory::Loot
