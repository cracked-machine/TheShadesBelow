#include <Components/AnimData.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/NoRender.hpp>
#include <Components/Position.hpp>
#include <Components/ZOrderValue.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/VertexFloor.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/Render/RenderPassTypes.hpp>
#include <Systems/Render/ZOrderQueue.hpp>
#include <Systems/ShaderSystem.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Profiling.hpp>

#include <algorithm>
#include <entt/entity/registry.hpp>

namespace Game::Sys
{

void ZOrderQueue::refresh( entt::registry &reg, sf::FloatRect view_bounds, const PathFinding::SpatialHashGridSharedPtr &render_position_grid )
{
  m_entries.clear();

  PROFILED( queue_multiblocks( reg, view_bounds, RenderPass::MultiBlockRoots{} ) );
  PROFILED( queue_all<Sprites::Containers::VertexFloor>( reg ) );
  PROFILED( queue_particles( reg, view_bounds ) );
  PROFILED( queue_all<Cmp::Shader::SpriteOwner>( reg ) );
  // inventory slots only have a zorder when they draw something in the world, e.g. dowsing rod guide lines
  PROFILED( queue_all<Cmp::PlayerInventorySlot>( reg ) );
  PROFILED( queue_positioned( reg, view_bounds, render_position_grid ) );

  PROFILED( std::ranges::sort( m_entries, {}, &Entry::z ) );
}

template <typename... MultiBlock>
void ZOrderQueue::queue_multiblocks( entt::registry &reg, sf::FloatRect view_bounds, entt::type_list<MultiBlock...> /*unused*/ )
{
  auto queue_visible = [&]<typename CmpT>()
  {
    auto view = reg.view<CmpT, Cmp::ZOrderValue>( entt::exclude<Cmp::NoRender> );
    for ( auto [entity, multiblock_cmp, z_order_cmp] : view.each() )
    {
      if ( Utils::is_visible_in_view( view_bounds, multiblock_cmp ) ) push( entity, z_order_cmp );
    }
  };
  ( queue_visible.template operator()<MultiBlock>(), ... );
}

template <typename CmpT>
void ZOrderQueue::queue_all( entt::registry &reg )
{
  auto view = reg.view<CmpT, Cmp::ZOrderValue>( entt::exclude<Cmp::NoRender> );
  for ( auto [entity, cmp, z_order_cmp] : view.each() )
  {
    push( entity, z_order_cmp );
  }
}

void ZOrderQueue::queue_particles( entt::registry &reg, sf::FloatRect view_bounds )
{
  auto view = reg.view<Cmp::Particle::SpriteOwner, Cmp::ZOrderValue>( entt::exclude<Cmp::NoRender> );
  for ( auto [entity, owner_cmp, z_order_cmp] : view.each() )
  {
    if ( owner_cmp.sprite && owner_cmp.sprite->get_view_type() == Cmp::Particle::ViewType::WORLD &&
         not Utils::is_visible_in_view( view_bounds, owner_cmp.sprite->get_bounds() ) )
    {
      continue;
    }
    push( entity, z_order_cmp );
  }
}

void ZOrderQueue::queue_positioned( entt::registry &reg, sf::FloatRect view_bounds,
                                    const PathFinding::SpatialHashGridSharedPtr &render_position_grid )
{
  if ( not render_position_grid )
  {
    queue_positioned_view( reg, view_bounds );
    return;
  }

  // The grid is only rebuilt periodically, so its entities may have been destroyed or changed since
  for ( auto entity : render_position_grid->query_rect( view_bounds ) )
  {
    if ( not reg.valid( entity ) or reg.all_of<Cmp::NoRender>( entity ) ) continue;
    auto *pos_cmp = reg.try_get<Cmp::Position>( entity );
    auto *z_order_cmp = reg.try_get<Cmp::ZOrderValue>( entity );
    if ( not pos_cmp or not z_order_cmp ) continue;
    if ( Utils::is_visible_in_view( view_bounds, *pos_cmp ) ) push( entity, *z_order_cmp );
  }

  [&]<typename... Mover>( entt::type_list<Mover...> ) { ( queue_positioned_view<Mover>( reg, view_bounds ), ... ); }( RenderPass::Movers{} );
}

template <typename... Filter>
void ZOrderQueue::queue_positioned_view( entt::registry &reg, sf::FloatRect view_bounds )
{
  auto view = reg.view<Cmp::Position, Cmp::AnimData, Cmp::ZOrderValue, Filter...>( RenderPass::exclude<RenderPass::MultiBlockRoots> );
  for ( auto entity : view )
  {
    auto [pos_cmp, z_order_cmp] = view.template get<Cmp::Position, Cmp::ZOrderValue>( entity );
    if ( Utils::is_visible_in_view( view_bounds, pos_cmp ) ) push( entity, z_order_cmp );
  }
}

void ZOrderQueue::push( entt::entity entity, const Cmp::ZOrderValue &z_order_cmp )
{
  m_entries.push_back( Entry{ .z = z_order_cmp.get(), .e = entity } );
}

} // namespace Game::Sys
