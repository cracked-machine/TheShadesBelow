#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Plant/BurningTimer.hpp>
#include <Components/Player/EatingTimeAccumulator.hpp>
#include <Components/Position.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/UUID.hpp>
#include <Factory/ActionFactory.hpp>
#include <Utils/Player.hpp>

namespace Game::Factory::Action
{

void try_eat_inventory( entt::registry &reg )
{
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg );
  if ( ( inventory_type.contains( ".forage" ) and Utils::Player::get_inventory_wear_level( reg ) > 0 ) or inventory_type.contains( ".rottenfood" ) )
  {
    auto player_entt = Utils::Player::get_entity( reg );
    if ( not reg.any_of<Cmp::Player::EatingTimeAccumulator>( player_entt ) )
    {
      reg.emplace_or_replace<Cmp::Player::EatingTimeAccumulator>( player_entt );
    }
  }
}

void try_burn_worlditem( entt::registry &reg )
{
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg );
  if ( inventory_type.contains( "candle" ) )
  {
    for ( auto [plant_entt, plant_cmp] : reg.view<Cmp::PlantMultiBlock>().each() )
    {
      if ( not Utils::Player::is_player_near( reg, plant_cmp ) ) continue;
      if ( not Utils::Player::get_projected_position( reg ).findIntersection( plant_cmp ) ) continue;

      if ( not reg.any_of<Cmp::Plant::BurningTimer>( plant_entt ) ) { reg.emplace_or_replace<Cmp::Plant::BurningTimer>( plant_entt ); }
    }
  }
}

} // namespace Game::Factory::Action
