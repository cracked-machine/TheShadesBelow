#include <Components/Altar/MultiBlock.hpp>
#include <Components/Crypt/RoomLavaPitCell.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/Wisp.hpp>
#include <Components/Particle/SpriteOwner.hpp>
#include <Components/Plant/BurningTimeAccumulator.hpp>
#include <Components/Player/TorchRadius.hpp>
#include <Components/UUID.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>

#include <algorithm>

namespace Game::Utils::Collision
{

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

  // the inventory candle's flame particle is paused (UI-space only) while carried, so it never
  // shows up in the Cmp::WorldItem loop below — check it explicitly, same as NightStaticShader does
  auto [inventory_entt, inventory_type, inventory_sprite_type] = Utils::Player::get_inventory( reg );
  if ( inventory_type.contains( "candle" ) ) { lights.circles.push_back( Utils::Player::get_position( reg ).getCenter() ); }

  for ( auto [candle_entt, candle_cmp, candle_pos] : reg.view<Cmp::WorldItem, Cmp::Position>().each() )
  {
    if ( not Utils::is_visible_in_view( light_bounds, candle_pos ) ) continue;
    if ( not candle_cmp.item_type.contains( "candle" ) ) continue;
    lights.circles.push_back( candle_pos.getCenter() );
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

  auto burning_plant_view = reg.view<Cmp::PlantMultiBlock, Cmp::Plant::BurningTimeAccumulator, Cmp::UUID>();
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