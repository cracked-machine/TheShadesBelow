#include <Components/AbsoluteAlpha.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Plant/Grass.hpp>
// #include <Components/PlantObstacle.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Position.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/DestroyPlantEvent.hpp>
#include <Factory/PlantFactory.hpp>
#include <Systems/BaseSystem.hpp>
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>
#include <entt/entity/registry.hpp>
#include <vector>

namespace Game::Factory::Plant
{

void remove_plant_mb( entt::registry &reg, entt::entity plant_entt )
{
  auto *plant_mb_cmp = reg.try_get<Cmp::PlantMultiBlock>( plant_entt );
  auto *plant_uuid_cmp = reg.try_get<Cmp::UUID>( plant_entt );
  if ( not plant_mb_cmp or not plant_uuid_cmp ) return;

  // Collect all segment entities for this plant, then destroy them.
  // Segment entities are purpose-built (not world tiles), so destroy rather than strip components.
  std::vector<entt::entity> to_destroy;
  for ( auto [other_uuid_entt, plant_segment_cmp, other_uuid_cmp] : reg.view<Cmp::PlantSegment, Cmp::UUID>().each() )
  {
    if ( *plant_uuid_cmp == other_uuid_cmp )
    {
      SPDLOG_DEBUG( "Found segment entt {}", static_cast<uint32_t>( other_uuid_entt ) );
      to_destroy.push_back( other_uuid_entt );
    }
  }
  for ( auto seg_entt : to_destroy )
  {
    if ( reg.valid( seg_entt ) ) reg.destroy( seg_entt );
  }

  // now destroy the redundant plant multiblock entity
  if ( reg.valid( plant_entt ) ) reg.destroy( plant_entt );
}

void destroy_grass_in( entt::registry &reg, const sf::FloatRect &area )
{
  // collect first: the event handler destroys the multiblock entities
  std::vector<entt::entity> grass;
  for ( auto [grass_entt, grass_mb_cmp] : reg.view<Cmp::Plant::Grass, Cmp::PlantMultiBlock>().each() )
  {
    if ( grass_mb_cmp.findIntersection( area ) ) grass.push_back( grass_entt );
  }
  for ( auto grass_entt : grass )
  {
    Sys::BaseSystem::get_systems_event_queue().trigger( Events::DestroyPlantEvent( grass_entt ) );
  }
}

} // namespace Game::Factory::Plant
