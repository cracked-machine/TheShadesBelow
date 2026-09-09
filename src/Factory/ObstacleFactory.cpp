#include <Components/AbsoluteAlpha.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/DestroyedObstacle.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Obstacle.hpp>
#include <Components/ObstacleCap.hpp>
#include <Components/Particle/BlockParticle.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Utils/Constants.hpp>
#include <entt/entity/fwd.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace Game::Factory::Obstacle
{

entt::entity create_world_pos( entt::registry &registry, const sf::Vector2f &pos )
{
  auto entity = registry.create();
  registry.emplace_or_replace<Cmp::Position>( entity, pos, Constants::kGridSizePxF );
  registry.emplace_or_replace<Cmp::Armable>( entity );
  return entity;
}

bool add_obstacle( entt::registry &reg, entt::entity entity, const PathFinding::SpatialHashGrid *reserved_sm )
{
  auto *pos_cmp = reg.try_get<Cmp::Position>( entity );
  if ( not pos_cmp ) return false;

  if ( ( reserved_sm != nullptr ) && not reserved_sm->at( *pos_cmp ).empty() ) return false;

  if ( reg.all_of<Cmp::DestroyedObstacle>( entity ) ) { reg.remove<Cmp::DestroyedObstacle>( entity ); }
  reg.emplace_or_replace<Cmp::Obstacle>( entity );
  reg.emplace_or_replace<Cmp::Npc::NoPathFinding>( entity );
  reg.emplace_or_replace<Cmp::Particle::BlockParticle>( entity );
  return true;
}

void add_obstacle_cap( entt::registry &reg, entt::entity entity ) { reg.emplace_or_replace<Cmp::ObstacleCap>( entity ); }

void decorate_obstacle( entt::registry &reg, entt::entity entity, Cmp::Position pos_cmp, const Sprites::SpriteSheet &ms, std::size_t sprite_tile_idx,
                        float zorder, bool blocking, const PathFinding::SpatialHashGrid *reserved_sm )
{
  if ( sprite_tile_idx > ms.get_sprite_count() - 1 )
  {
    throw std::runtime_error( "Unable to get index " + std::to_string( sprite_tile_idx ) + " in " + ms.get_sprite_type() +
                              " ( size: " + std::to_string( ms.get_sprite_count() ) + " )" );
  }
  Cmp::ZOrderValue zorder_cmp( 0 );

  // Use the non-zero function arg, or the non-zero json value, or fallback to the sprite y-axis
  if ( zorder != 0 ) { zorder_cmp.setZOrder( zorder ); }
  else if ( ms.get_zorder( sprite_tile_idx ) != 0 ) { zorder_cmp.setZOrder( ms.get_zorder( sprite_tile_idx ) ); }
  else { zorder_cmp.setZOrder( pos_cmp.position.y ); }

  if ( reg.any_of<Cmp::Player::Character>( entity ) ) { return; }
  if ( ( reserved_sm != nullptr ) && not reserved_sm->at( pos_cmp ).empty() ) { return; }
  if ( reg.all_of<Cmp::DestroyedObstacle>( entity ) ) { reg.remove<Cmp::DestroyedObstacle>( entity ); }
  reg.emplace_or_replace<Cmp::ZOrderValue>( entity, zorder );

  if ( blocking )
  {
    reg.emplace_or_replace<Cmp::Player::NoPath>( entity );
    reg.emplace_or_replace<Cmp::Particle::BlockParticle>( entity );
  }
  reg.emplace_or_replace<Cmp::AbsoluteAlpha>( entity, 255 );
  reg.emplace_or_replace<Cmp::Armable>( entity );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( entity, Cmp::AnimData::Config{ 
        .sprite_type = ms.get_sprite_type(), 
        .frame_index_offset = sprite_tile_idx,
        .enabled = true
  });
  // clang-format on
  reg.emplace_or_replace<Cmp::Armable>( entity );

  SPDLOG_DEBUG( "Added obstacle {} at [{},{}] Z: {}", ms.get_display_name(), pos_cmp.x(), pos_cmp.y(), zorder_cmp.getZOrder() );
}

void remove_obstacle( entt::registry &reg, entt::entity search_entt, DeleteExtras delete_extras,
                      const PathFinding::SpatialHashGridSharedPtr &reserved_sm )
{
  if ( not reg.valid( search_entt ) ) return;

  Cmp::UUID search_uuid_cmp;
  auto *search_uuid_cmp_ptr = reg.try_get<Cmp::UUID>( search_entt );
  if ( search_uuid_cmp_ptr ) { search_uuid_cmp = *search_uuid_cmp_ptr; }

  reg.remove<Cmp::Obstacle>( search_entt );
  reg.remove<Cmp::ZOrderValue>( search_entt );
  reg.remove<Cmp::Npc::NoPathFinding>( search_entt );
  reg.remove<Cmp::Player::NoPath>( search_entt );
  reg.remove<Cmp::Particle::BlockParticle>( search_entt );
  reg.remove<Cmp::AbsoluteAlpha>( search_entt );
  reg.remove<Cmp::AnimData>( search_entt );
  reg.remove<Cmp::UUID>( search_entt );
  SPDLOG_DEBUG( "Removing obstacle {} - {}", static_cast<uint32_t>( search_entt ), search_uuid_cmp.str() );

  // don't search for all zeroes UUID
  if ( search_uuid_cmp.empty() )
  {
    SPDLOG_DEBUG( "This obstacle {} does not have a matching cap obstacle {}", static_cast<uint32_t>( search_entt ), search_uuid_cmp.str() );
    return;
  }

  // delete any cap entity (tied to obstacle entt by UUID)
  for ( auto [cap_entt, cap_uuid_cmp] : reg.view<Cmp::UUID>().each() )
  {
    if ( cap_uuid_cmp != search_uuid_cmp ) continue;
    SPDLOG_DEBUG( "Removing matching obstacle {} - {}", static_cast<uint32_t>( cap_entt ), search_uuid_cmp.str() );
    if ( delete_extras == DeleteExtras::Yes )
    {
      // capture position before destroy - registry::destroy can invalidate component references via swap-and-pop
      if ( reserved_sm )
      {
        if ( auto *cap_pos_cmp = reg.try_get<Cmp::Position>( cap_entt ) ) reserved_sm->remove( cap_entt, *cap_pos_cmp );
      }
      reg.destroy( cap_entt );
    }
  }
}

void remove_obstacle( entt::registry &reg, entt::entity search_entt, DeleteExtras delete_extras, const UUIDEntityMap &uuid_map,
                      const PathFinding::SpatialHashGridSharedPtr &reserved_sm )
{
  if ( not reg.valid( search_entt ) ) return;

  Cmp::UUID search_uuid_cmp;
  auto *search_uuid_cmp_ptr = reg.try_get<Cmp::UUID>( search_entt );
  if ( search_uuid_cmp_ptr ) { search_uuid_cmp = *search_uuid_cmp_ptr; }

  reg.remove<Cmp::Obstacle>( search_entt );
  reg.remove<Cmp::ZOrderValue>( search_entt );
  reg.remove<Cmp::Npc::NoPathFinding>( search_entt );
  reg.remove<Cmp::Player::NoPath>( search_entt );
  reg.remove<Cmp::Particle::BlockParticle>( search_entt );
  reg.remove<Cmp::AbsoluteAlpha>( search_entt );
  reg.remove<Cmp::AnimData>( search_entt );
  reg.remove<Cmp::UUID>( search_entt );
  SPDLOG_DEBUG( "Removing obstacle {} - {}", static_cast<uint32_t>( search_entt ), search_uuid_cmp.str() );

  if ( delete_extras == DeleteExtras::No || search_uuid_cmp.empty() ) return;

  // O(1) cap entity lookup — map contains only cap entities (UUID present, no Obstacle)
  auto it = uuid_map.find( search_uuid_cmp );
  if ( it != uuid_map.end() && reg.valid( it->second ) )
  {
    SPDLOG_DEBUG( "Removing matching cap entity {} - {}", static_cast<uint32_t>( it->second ), search_uuid_cmp.str() );
    if ( reserved_sm )
    {
      if ( auto *cap_pos_cmp = reg.try_get<Cmp::Position>( it->second ) ) reserved_sm->remove( it->second, *cap_pos_cmp );
    }
    reg.destroy( it->second );
    return;
  }

  // Fallback: cap was created after the map was built (e.g. mid-shuffle decorate_interior_wall)
  for ( auto [cap_entt, cap_uuid_cmp] : reg.view<Cmp::UUID>().each() )
  {
    if ( cap_uuid_cmp != search_uuid_cmp ) continue;
    SPDLOG_DEBUG( "Removing cap entity (fallback scan) {} - {}", static_cast<uint32_t>( cap_entt ), search_uuid_cmp.str() );
    if ( reserved_sm )
    {
      if ( auto *cap_pos_cmp = reg.try_get<Cmp::Position>( cap_entt ) ) reserved_sm->remove( cap_entt, *cap_pos_cmp );
    }
    reg.destroy( cap_entt );
    break;
  }
}

} // namespace Game::Factory::Obstacle
