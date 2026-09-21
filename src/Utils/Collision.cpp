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

namespace Game::Utils::Collision
{

bool is_position_illuminated( entt::registry &reg, const Cmp::Position &pos_cmp )
{
  const auto torch_radius = Utils::Player::get_torch_radius( reg );

  // the inventory candle's flame particle is paused (UI-space only) while carried, so it never
  // shows up in the Cmp::WorldItem loop below — check it explicitly, same as NightStaticShader does
  auto [inventory_entt, inventory_type, inventory_sprite_type] = Utils::Player::get_inventory( reg );
  if ( inventory_type.contains( "candle" ) )
  {
    float distance = Utils::Maths::getEuclideanDistance( Utils::Player::get_position( reg ).getCenter(), pos_cmp.getCenter() );
    if ( distance <= torch_radius.value ) return true;
  }

  for ( auto [candle_entt, candle_cmp, candle_pos] : reg.view<Cmp::WorldItem, Cmp::Position>().each() )
  {
    if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), candle_pos ) ) continue;
    if ( not candle_cmp.item_type.contains( "candle" ) ) continue;
    float distance = Utils::Maths::getEuclideanDistance( candle_pos.getCenter(), pos_cmp.getCenter() );
    if ( distance > torch_radius.value ) continue;
    return true;
  }

  for ( auto [altar_entt, altar_cmp, altar_uuid_cmp] : reg.view<Cmp::Altar::MultiBlock, Cmp::UUID>().each() )
  {
    if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), altar_cmp ) ) continue;
    for ( auto [particle_entt, particle_cmp, particle_uuid_cmp] : reg.view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
    {
      if ( altar_uuid_cmp != particle_uuid_cmp ) continue;

      float distance = Utils::Maths::getEuclideanDistance( particle_cmp.sprite->get_emitter_position(), pos_cmp.getCenter() );
      if ( distance > torch_radius.value ) continue;
      return true;
    }
  }

  for ( auto [lava_entt, lava_cmp] : reg.view<Cmp::Crypt::RoomLavaPitCell>().each() )
  {
    if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), lava_cmp ) ) continue;
    auto candidate_hitbox = Cmp::RectBounds::scaled( pos_cmp.position, Constants::kGridSizePxF, 1.5f );
    if ( candidate_hitbox.findIntersection( lava_cmp ) ) return true;
  }

  auto burning_plant_view = reg.view<Cmp::PlantMultiBlock, Cmp::Plant::BurningTimeAccumulator, Cmp::UUID>();
  for ( auto [plant_entt, plant_cmp, plant_burn_cmp, plant_uuid_cmp] : burning_plant_view.each() )
  {
    if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), plant_cmp ) ) continue;
    for ( auto [particle_entt, particle_cmp, particle_uuid_cmp] : reg.view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
    {
      if ( plant_uuid_cmp != particle_uuid_cmp ) continue;

      float distance = Utils::Maths::getEuclideanDistance( particle_cmp.sprite->get_emitter_position(), pos_cmp.getCenter() );
      if ( distance > torch_radius.value ) continue;
      return true;
    }
  }

  for ( auto [npc_entt, npc_cmp, npc_pos_cmp] : reg.view<Cmp::Npc::NPC, Cmp::Position>().each() )
  {
    if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), npc_pos_cmp ) ) continue;
    if ( not reg.any_of<Cmp::Npc::Wisp>( npc_entt ) ) continue;

    float distance = Utils::Maths::getEuclideanDistance( npc_pos_cmp.getCenter(), pos_cmp.getCenter() );
    if ( distance > torch_radius.value ) continue;
    return true;
  }

  return false;
}

} // namespace Game::Utils::Collision