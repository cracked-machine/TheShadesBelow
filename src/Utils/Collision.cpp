#include <Components/Altar/MultiBlock.hpp>
#include <Components/Crypt/RoomLavaPitCell.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Hazard/SinkholeCell.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Npc/Burryman.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/Wisp.hpp>
#include <Components/Particle/SpriteOwner.hpp>
#include <Components/Persistent/HazardBalanceOverlap.hpp>
#include <Components/Plant/BurningTimer.hpp>
#include <Components/Player/TorchRadius.hpp>
#include <Components/UUID.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>

#include <algorithm>

namespace Game::Utils::Collision
{

Cmp::RectBounds lethal_hazard_probe( entt::registry &reg, const sf::FloatRect &rect )
{
  const float overlap = std::clamp( Sys::PersistSystem::get<Cmp::Persist::HazardBalanceOverlap>( reg ).get_value(), 0.f, 0.5f );
  // keep at least 1px so the probe never shrinks to nothing (SFML never intersects an empty rect)
  const sf::Vector2f probe_size( std::max( rect.size.x * ( 1.f - 2.f * overlap ), 1.f ), std::max( rect.size.y * ( 1.f - 2.f * overlap ), 1.f ) );
  return Cmp::RectBounds::scaled( sf::FloatRect( rect.getCenter() - probe_size * 0.5f, probe_size ), 1.f );
}

entt::entity find_lethal_hazard( entt::registry &reg, const Cmp::RectBounds &rect )
{
  for ( auto [sinkhole_entt, sinkhole_cmp, sinkhole_pos] : reg.view<Cmp::SinkholeCell, Cmp::Position>().each() )
  {
    if ( rect.findIntersection( sinkhole_pos ) ) return sinkhole_entt;
  }
  for ( auto [lava_entt, lava_cmp] : reg.view<Cmp::Crypt::RoomLavaPitCell>().each() )
  {
    if ( rect.findIntersection( lava_cmp ) ) return lava_entt;
  }
  return entt::null;
}

LightSources collect_light_sources( entt::registry &reg )
{
  LightSources lights;
  lights.radius = Utils::Player::get_torch_radius( reg ).value;

  // Pad the view by the light's reach so a source just off-screen whose glow extends on-screen still counts.
  // Lava lights a 1.5x grid hitbox around the tested cell, so pad by at least one grid cell for those.
  const float reach = std::max( lights.radius, Constants::kGridSizePxF.x );
  auto light_bounds = Utils::calculate_view_bounds( Sys::RenderSystem::get_world_view() );
  light_bounds.position -= sf::Vector2f( reach, reach );
  light_bounds.size += sf::Vector2f( reach * 2.f, reach * 2.f );

  // the inventory torch's flame particle is paused (UI-space only) while carried, so it never
  // shows up in the Cmp::WorldItem loop below — check it explicitly, same as NightStaticShader does
  // a torch in hand, or a stowed one when Cmp::Persist::StowedTorchLit is on
  if ( Utils::Player::has_lit_torch( reg ) ) { lights.circles.push_back( Utils::Player::get_position( reg ).getCenter() ); }

  for ( auto [torch_entt, torch_cmp, torch_pos] : reg.view<Cmp::WorldItem, Cmp::Position>().each() )
  {
    if ( not Utils::is_visible_in_view( light_bounds, torch_pos ) ) continue;
    if ( not torch_cmp.item_type.contains( "torch" ) ) continue;
    lights.circles.push_back( torch_pos.getCenter() );
  }

  // a torch the Burryman has stolen lights his way
  for ( auto [burryman_entt, burryman_cmp, burryman_pos] : reg.view<Cmp::Npc::Burryman, Cmp::Position>().each() )
  {
    if ( not Utils::is_visible_in_view( light_bounds, burryman_pos ) ) continue;
    const bool has_torch = std::ranges::any_of( burryman_cmp.carried,
                                                 [&]( entt::entity item_entt )
                                                 {
                                                   if ( not reg.valid( item_entt ) ) return false;
                                                   const auto *item_cmp = reg.try_get<Cmp::WorldItem>( item_entt );
                                                   return item_cmp and item_cmp->item_type.contains( "torch" );
                                                 } );
    if ( has_torch ) lights.circles.push_back( burryman_pos.getCenter() );
  }

  for ( auto [altar_entt, altar_cmp, altar_uuid_cmp] : reg.view<Cmp::Altar::MultiBlock, Cmp::UUID>().each() )
  {
    if ( not Utils::is_visible_in_view( light_bounds, altar_cmp ) ) continue;
    for ( auto [particle_entt, particle_cmp, particle_uuid_cmp] : reg.view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
    {
      if ( altar_uuid_cmp != particle_uuid_cmp ) continue;
      lights.circles.push_back( particle_cmp.sprite->get_emitter_position() );
    }
  }

  for ( auto [lava_entt, lava_cmp] : reg.view<Cmp::Crypt::RoomLavaPitCell>().each() )
  {
    if ( not Utils::is_visible_in_view( light_bounds, lava_cmp ) ) continue;
    lights.lava.push_back( lava_cmp );
  }

  auto burning_plant_view = reg.view<Cmp::PlantMultiBlock, Cmp::Plant::BurningTimer, Cmp::UUID>();
  for ( auto [plant_entt, plant_cmp, plant_burn_cmp, plant_uuid_cmp] : burning_plant_view.each() )
  {
    if ( not Utils::is_visible_in_view( light_bounds, plant_cmp ) ) continue;
    for ( auto [particle_entt, particle_cmp, particle_uuid_cmp] : reg.view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
    {
      if ( plant_uuid_cmp != particle_uuid_cmp ) continue;
      lights.circles.push_back( particle_cmp.sprite->get_emitter_position() );
    }
  }

  for ( auto [npc_entt, npc_cmp, npc_pos_cmp] : reg.view<Cmp::Npc::NPC, Cmp::Position>().each() )
  {
    if ( not Utils::is_visible_in_view( light_bounds, npc_pos_cmp ) ) continue;
    if ( not reg.any_of<Cmp::Npc::Wisp>( npc_entt ) ) continue;
    lights.circles.push_back( npc_pos_cmp.getCenter() );
  }

  return lights;
}

bool LightSources::circle_lights( const sf::Vector2f &centre, const Cmp::Position &pos ) const
{
  return Utils::Maths::getEuclideanDistance( centre, pos.getCenter() ) <= radius;
}

bool LightSources::lava_lights( const Cmp::Position &lava_cell, const Cmp::Position &pos )
{
  auto candidate_hitbox = Cmp::RectBounds::scaled( pos.position, Constants::kGridSizePxF, 1.5f );
  return candidate_hitbox.findIntersection( lava_cell ).has_value();
}

bool LightSources::illuminates( const Cmp::Position &pos ) const
{
  for ( const auto &centre : circles )
    if ( circle_lights( centre, pos ) ) return true;
  for ( const auto &lava_cell : lava )
    if ( lava_lights( lava_cell, pos ) ) return true;
  return false;
}

bool LightSources::blocks( const Cmp::Position &pos, std::span<const Cmp::Position> exempt ) const
{
  auto lights_any_exempt = [&]( auto &&lights_fn ) { return std::ranges::any_of( exempt, lights_fn ); };

  for ( const auto &centre : circles )
  {
    if ( not circle_lights( centre, pos ) ) continue;
    if ( lights_any_exempt( [&]( const Cmp::Position &e ) { return circle_lights( centre, e ); } ) ) continue;
    return true;
  }
  for ( const auto &lava_cell : lava )
  {
    if ( not lava_lights( lava_cell, pos ) ) continue;
    if ( lights_any_exempt( [&]( const Cmp::Position &e ) { return lava_lights( lava_cell, e ); } ) ) continue;
    return true;
  }
  return false;
}

bool is_position_illuminated( entt::registry &reg, const Cmp::Position &pos_cmp ) { return collect_light_sources( reg ).illuminates( pos_cmp ); }

} // namespace Game::Utils::Collision