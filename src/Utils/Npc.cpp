#include <Components/AnimData.hpp>
#include <Components/Direction.hpp>
#include <Components/FootStepTimer.hpp>
#include <Components/LerpPosition.hpp>
#include <Components/Npc/LerpSpeed.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Position.hpp>
#include <PathFinding/AStar.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/SpriteMetaType.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Npc.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <entt/entity/fwd.hpp>
#include <source_location>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace Game::Utils::Npc
{

entt::entity get_world_pos_entt( entt::registry &reg, Cmp::Position npc_pos )
{

  auto excl = entt::exclude<Cmp::Player::Character, Cmp::Npc::NPC, Cmp::FootStepTimer>;
  for ( auto [world_entt, world_pos_cmp] : reg.view<Cmp::Position>( excl ).each() )
  {
    if ( npc_pos.findIntersection( world_pos_cmp ) ) return world_entt;
  }
  return entt::null;
}

entt::entity get_world_pos_entt( entt::registry &reg, entt::entity npc_entt )
{
  auto *npc_pos = reg.try_get<Cmp::Position>( npc_entt );
  if ( not npc_pos ) return entt::null;

  auto excl = entt::exclude<Cmp::Player::Character, Cmp::Npc::NPC, Cmp::FootStepTimer>;
  for ( auto [world_entt, world_pos_cmp] : reg.view<Cmp::Position>( excl ).each() )
  {
    if ( npc_pos->findIntersection( world_pos_cmp ) ) return world_entt;
  }
  return entt::null;
}

Sprites::SpriteMetaType get_sprite_type( entt::registry &reg, entt::entity npc_entt, std::source_location loc )
{
  auto loc_data = std::string( loc.file_name() ) + ":" + std::to_string( loc.line() ) + " - ";
  auto *anim_cmp = reg.try_get<Cmp::AnimData>( npc_entt );
  if ( not anim_cmp )
    throw std::runtime_error( loc_data + "Could not get AnimData component from " + std::to_string( static_cast<uint32_t>( npc_entt ) ) );
  return anim_cmp->m_sprite_type;
}

PathfindResult pathfind_toward( entt::registry &reg, PathFinding::SpatialHashGrid &navmesh, const Cmp::Position &target_pos, entt::entity npc_entity,
                                bool target_in_spawn, bool target_illuminated, bool always_pathfind )
{
  auto *npc_anim_cmp = reg.try_get<Cmp::AnimData>( npc_entity );
  if ( not npc_anim_cmp ) return PathfindResult::Blocked;
  auto npc_type = npc_anim_cmp->m_sprite_type;

  auto *npc_pos_cmp = reg.try_get<Cmp::Position>( npc_entity );
  if ( not npc_pos_cmp ) return PathfindResult::Blocked;

  // only pathfind when NPC is in the current view/screen - except wisps - they need to pathfind at all times.
  if ( not always_pathfind and not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), *npc_pos_cmp ) )
  {
    reg.emplace_or_replace<Cmp::Direction>( npc_entity, Cmp::Direction( { 0.0, 0.0 } ) );
    return PathfindResult::Blocked;
  }

  // don't intterupt NPC mid-lerp or it causes indecisive pathfinding
  auto *npc_lerp_pos_cmp = reg.try_get<Cmp::LerpPosition>( npc_entity );
  if ( npc_lerp_pos_cmp && npc_lerp_pos_cmp->m_lerp_factor < 1.0f ) return PathfindResult::Blocked;

  // allow ghosts to sneak through corners
  auto query_compass = PathFinding::QueryCompass::CARDINAL;

  if ( npc_type.contains( "sprite.ghost" ) ) query_compass = PathFinding::QueryCompass::BOTH;

  // Snap goal to cell top-left: player moves sub-grid so can appear up to 31px into an
  // adjacent cell, exceeding the 24px too_far threshold when NPC approaches from left/above.
  Cmp::Position grid_target( Utils::snap_to_grid( target_pos.position, Utils::Rounding::TOWARDS_ZERO ), target_pos.size );

  std::vector<PathFinding::PathNode> path;
  path = PathFinding::astar( reg, navmesh, *npc_pos_cmp, grid_target, query_compass );

  SPDLOG_DEBUG( "{} pathsize: {}", static_cast<uint32_t>( npc_entity ), path.size() );
  if ( path.size() <= 1 ) return PathfindResult::NoPath;

  Cmp::Position next_npc_pos = path[1].pos;

  // If player is in spawn, only stop when the very next step would cross into spawn.
  // This lets the NPC walk the full path to the boundary before stopping.
  if ( target_in_spawn and Utils::Player::is_in_spawn( reg, next_npc_pos ) )
  {
    reg.emplace_or_replace<Cmp::Direction>( npc_entity, Cmp::Direction( { 0.0f, 0.0f } ) );
    return PathfindResult::Blocked;
  }

  // If the player is illuminated, only stop when the very next step would cross into the radius of
  // whichever light source is currently illuminating them. This lets the NPC walk the full path to
  // that light's boundary before stopping, mirroring the spawn-boundary check above.
  if ( target_illuminated and Utils::Collision::is_position_illuminated( reg, next_npc_pos ) )
  {
    reg.emplace_or_replace<Cmp::Direction>( npc_entity, Cmp::Direction( { 0.0f, 0.0f } ) );
    return PathfindResult::Blocked;
  }

  // calculate the direction and update the NPC lerp
  auto *npc_lerp_speed_cmp = reg.try_get<Cmp::Npc::LerpSpeed>( npc_entity );
  if ( not npc_lerp_speed_cmp ) return PathfindResult::Blocked;
  auto candidate_lerp_pos = Cmp::LerpPosition( next_npc_pos.position, npc_lerp_speed_cmp->speed );
  auto distance_to_target = next_npc_pos.position - npc_pos_cmp->position;
  if ( distance_to_target == sf::Vector2f( 0.0f, 0.0f ) ) return PathfindResult::Blocked;

  // prevent NPC warping via another NPCs pathfinding
  const bool too_far = std::abs( distance_to_target.x ) >= Constants::kGridSizePxF.x * 1.5f ||
                       std::abs( distance_to_target.y ) >= Constants::kGridSizePxF.y * 1.5f;
  if ( too_far ) return PathfindResult::Blocked;

  auto norm_direction = Cmp::Direction( distance_to_target.normalized() );

  reg.emplace_or_replace<Cmp::Direction>( npc_entity, norm_direction );
  reg.emplace_or_replace<Cmp::LerpPosition>( npc_entity, candidate_lerp_pos );
  return PathfindResult::Moved;
}

} // namespace Game::Utils::Npc