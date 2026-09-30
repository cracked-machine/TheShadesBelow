#include <Components/Altar/MultiBlock.hpp>
#include <Components/AnimData.hpp>
#include <Components/Crypt/BuildingMultiBlock.hpp>
#include <Components/Crypt/InteriorMultiBlock.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Moveable.hpp>
#include <Components/NoRender.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/ObstacleCap.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Position.hpp>
#include <Components/Ruin/BuildingMultiBlock.hpp>
#include <Components/Spring/HealingSpringBuildingMultiBlock.hpp>
#include <Components/Weapons/Arrow.hpp>
#include <Components/ZOrderValue.hpp>
#include <Factory/PathfindingFactory.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/Render/RenderPassTypes.hpp>

namespace Game::Factory::Pathfinding
{

PathFinding::SpatialHashGridSharedPtr create_npc_navmesh( entt::registry &reg )
{

  // first pass: find all navigable positions
  PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = std::make_shared<PathFinding::SpatialHashGrid>();
  for ( auto [pos_entt, pos_cmp] : reg.view<Cmp::Position>().each() )
  {
    pathfinding_navmesh->insert( pos_entt, pos_cmp );
  }
  // second pass: If we find any position with a Cmp::Npc::NoPathFinding then that invalidates the entire bucket at that position.
  for ( auto [pos_entt, pos_cmp] : reg.view<Cmp::Position>().each() )
  {
    if ( reg.all_of<Cmp::Npc::NoPathFinding>( pos_entt ) ) { pathfinding_navmesh->remove_all( pos_cmp ); }
  }
  return pathfinding_navmesh;
}

PathFinding::SpatialHashGridSharedPtr create_ghost_navmesh( entt::registry &reg )
{
  // first pass: find all navigable positions
  PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = std::make_shared<PathFinding::SpatialHashGrid>();
  for ( auto [pos_entt, pos_cmp] : reg.view<Cmp::Position>().each() )
  {
    pathfinding_navmesh->insert( pos_entt, pos_cmp );
  }
  // second pass: If we find any position with a Cmp::Npc::NoPathFinding then that invalidates the entire bucket at that position.
  for ( auto [pos_entt, pos_cmp] : reg.view<Cmp::Position>().each() )
  {
    if ( reg.all_of<Cmp::Npc::NoPathFinding>( pos_entt ) ) { pathfinding_navmesh->remove_all( pos_cmp ); }
  }
  return pathfinding_navmesh;
}

PathFinding::SpatialHashGridSharedPtr create_player_navmesh( entt::registry &reg )
{
  // Index only the blocking obstacles so is_valid_move can do a spatial lookup
  // instead of scanning every NoPath entity in the scene.
  PathFinding::SpatialHashGridSharedPtr player_navmesh = std::make_shared<PathFinding::SpatialHashGrid>();
  for ( auto [pos_entt, pos_cmp] : reg.view<Cmp::Player::NoPath, Cmp::Position>().each() )
  {
    player_navmesh->insert( pos_entt, pos_cmp );
  }
  return player_navmesh;
}

PathFinding::SpatialHashGridSharedPtr create_open_navmesh( entt::registry &reg )
{
  // create a navmesh for uninhibited pathfinding
  PathFinding::SpatialHashGridSharedPtr open_navmesh = std::make_shared<PathFinding::SpatialHashGrid>();
  for ( auto [pos_entt, pos_cmp] : reg.view<Cmp::Position>().each() )
  {
    open_navmesh->insert( pos_entt, pos_cmp );
  }
  return open_navmesh;
}

void populate_render_position_grid( entt::registry &reg, PathFinding::SpatialHashGrid &render_position_grid )
{
  render_position_grid.clear();

  // Movers are excluded because RenderGameSystem::queue_positioned() scans them separately every frame;
  // multiblock roots are excluded because they already get their own dedicated z-order pass.
  auto static_render_view = reg.view<Cmp::Position, Cmp::AnimData, Cmp::ZOrderValue>(
      Sys::RenderPass::exclude<Sys::RenderPass::Movers, Sys::RenderPass::MultiBlockRoots> );
  for ( auto entity : static_render_view )
  {
    const auto &pos_cmp = static_render_view.get<Cmp::Position>( entity );
    render_position_grid.insert( entity, pos_cmp );
  }
}

PathFinding::SpatialHashGridSharedPtr create_render_position_grid( entt::registry &reg )
{
  PathFinding::SpatialHashGridSharedPtr render_position_grid = std::make_shared<PathFinding::SpatialHashGrid>();
  populate_render_position_grid( reg, *render_position_grid );
  return render_position_grid;
}

} // namespace Game::Factory::Pathfinding