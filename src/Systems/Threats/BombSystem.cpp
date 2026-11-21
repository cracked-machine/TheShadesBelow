#include <Audio/SoundBank.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/Armed.hpp>
#include <Components/BlastMarker.hpp>
#include <Components/Crypt/BuildingSegment.hpp>
#include <Components/DeathPosition.hpp>
#include <Components/DestroyedObstacle.hpp>
#include <Components/Exit.hpp>
#include <Components/Grave/Exhumed.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Grave/Segment.hpp>
#include <Components/Inventory/BlastProof.hpp>
#include <Components/ObstacleCap.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LootContainer.hpp>
#include <Components/Npc/Container.hpp>
#include <Components/Npc/Crow.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Persistent/ArmedOffDelay.hpp>
#include <Components/Persistent/BombDamage.hpp>
#include <Components/Persistent/BombsExhumeGraves.hpp>
#include <Components/Persistent/ReservedArmedBlockColourBorder.hpp>
#include <Components/Persistent/ReservedArmedBlockColourFill.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/DestroyLootContainerEvent.hpp>
#include <Events/DestroyitemEvent.hpp>
#include <Events/DestroyNpcEvent.hpp>
#include <Events/ExhumeGraveEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <Factory/BombFactory.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Threats/BombSystem.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <spdlog/spdlog.h>
#include <utility>
#include <vector>

namespace
{
//! @brief Points of player luck per tile of player bomb blast radius.
constexpr int kLuckPerBlastTile = 20;

//! @brief Grave bomb blast radius (tiles) at zero luck.
constexpr int kGraveBlastMaxRadius = 3;
//! @brief Smallest grave bomb blast radius (tiles), reached at kGraveLuckPerBlastTile * (max - min) luck and above.
//! 0 still detonates the epicenter tile itself (hurting the player if they are on it) but no rings around it, and
//! does not exhume neighbouring graves.
constexpr int kGraveBlastMinRadius = 1;
//! @brief Points of player luck per tile the grave bomb blast radius shrinks by.
//! Resulting grave bomb blast radius (luck: radius) with the values above:
//! - 0-9: 3
//! - 10-19: 2
//! - 20-100: 1
constexpr int kGraveLuckPerBlastTile = 10;
} // namespace

namespace Game::Sys
{
using entt::exclude;

BombSystem::BombSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  // The entt::dispatcher is independent of the registry, so it is safe to bind event handlers in
  // the constructor
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&Sys::BombSystem::on_bomb_event>( this );
  SPDLOG_DEBUG( "BombSystem initialized" );
}

void BombSystem::update( sf::Time dt )
{
  for ( auto [armed_entt, armed_cmp] : reg().view<Cmp::Armed>().each() )
  {
    armed_cmp.m_elapsed += dt;
  }

  auto pathfinding_navmesh = m_npc_navmesh.lock();
  auto player_navmesh = m_player_navmesh.lock();
  if ( not pathfinding_navmesh or not player_navmesh )
  {
    SPDLOG_WARN( "Unable to lock navmesh weakptr (npc: {}, player: {})", bool( pathfinding_navmesh ), bool( player_navmesh ) );
    return;
  }

  auto reserved_sm = m_reserved_sm.lock();

  // Snapshot the armed entities up front rather than holding armed_view's iterator open across the loop body.
  // Detonating one bomb can, via chain reactions and reentrant events (grave BOMB_TRAP/JEWELRY consequences,
  // NPC loot drops), arm further entities (growing Cmp::Armed) and create new Position/AnimData-bearing
  // entities - both reallocate the pools this view is built from, which would invalidate a live iterator.
  // A plain vector can't be invalidated by any of that.
  std::vector<entt::entity> armed_entities;
  for ( auto armed_entt : reg().view<Cmp::Armed, Cmp::Position>() )
    armed_entities.push_back( armed_entt );

  for ( entt::entity armed_entt : armed_entities )
  {
    auto *armed_cmp_ptr = reg().try_get<Cmp::Armed>( armed_entt );
    auto *armed_pos_cmp_ptr = reg().try_get<Cmp::Position>( armed_entt );
    if ( not armed_cmp_ptr or not armed_pos_cmp_ptr ) continue; // removed/changed already this tick (e.g. a chain reaction)
    if ( armed_cmp_ptr->getElapsedFuseTime() < armed_cmp_ptr->m_fuse_delay ) continue;

    // visual-only tile over a reserved position: its flashing is done, nothing to detonate
    if ( reg().all_of<Cmp::BlastMarker>( armed_entt ) )
    {
      reg().destroy( armed_entt );
      continue;
    }

    // Copy out what the rest of this iteration needs now, before any mutating calls below: those calls can
    // reallocate the Cmp::Armed/Cmp::Position pools, which would dangle armed_cmp_ptr/armed_pos_cmp_ptr.
    Cmp::Position armed_pos_cmp = *armed_pos_cmp_ptr;
    bool is_epicenter = ( armed_cmp_ptr->m_epicenter == Cmp::Armed::EpiCenter::YES );
    const int blast_radius = armed_cmp_ptr->m_blast_radius;

    // exhume every closed grave within the blast radius. Snapshot first: the handlers spawn loot/NPCs and can
    // arm further grave bombs, all of which reallocate the pools a live view would iterate.
    if ( is_epicenter and blast_radius > 0 and Sys::PersistSystem::get<Cmp::Persist::BombsExhumeGraves>( reg() ).get_value() )
    {
      const sf::Vector2f reach = Constants::kGridSizePxF * static_cast<float>( blast_radius );
      const sf::FloatRect blast_area( armed_pos_cmp.position - reach, armed_pos_cmp.size + ( reach * 2.f ) );
      std::vector<entt::entity> blasted_graves;
      for ( auto [grave_entt, grave_cmp] : reg().view<Cmp::Grave::MultiBlock>( exclude<Cmp::Grave::Exhumed> ).each() )
      {
        if ( blast_area.findIntersection( grave_cmp ) ) blasted_graves.push_back( grave_entt );
      }
      for ( entt::entity grave_entt : blasted_graves )
      {
        get_systems_event_queue().trigger( Events::ExhumeGraveEvent( grave_entt ) );
      }
    }

    // detonate obstacles - remove all traces of obstacle
    Utils::Collision::for_each_intersect<Cmp::Obstacle>( reg(), armed_pos_cmp,
                                                         [&]( entt::entity obst_entity, Cmp::Obstacle &, Cmp::Position &obst_pos_ref )
    {
      // copied: remove_obstacle() destroys the cap entity, which can swap-and-pop the Position pool under the reference
      const Cmp::Position obst_pos_cmp = obst_pos_ref;
      if ( reserved_sm && not reserved_sm->at( obst_pos_cmp ).empty() ) return;
      Factory::Obstacle::remove_obstacle( reg(), obst_entity, Factory::Obstacle::DeleteExtras::Yes, reserved_sm );
      pathfinding_navmesh->insert( obst_entity, obst_pos_cmp );
      player_navmesh->insert( obst_entity, obst_pos_cmp );
      if ( auto ghost_navmesh = m_ghost_navmesh.lock() ) { ghost_navmesh->insert( obst_entity, obst_pos_cmp ); }
    } );

    // detonate loot containers - breaking them is handled by LootSystem. Collect first and trigger afterwards:
    // LootSystem spawns loot with a Cmp::Position, which must not happen while iterating the Position view.
    std::vector<entt::entity> detonated_loot_containers;
    // Cmp::LootContainer is an empty tag type, so it can't be used with Utils::Collision::for_each_intersect
    for ( auto [loot_entt, loot_pos_cmp] : reg().view<Cmp::LootContainer, Cmp::Position>().each() )
    {
      if ( armed_pos_cmp.findIntersection( loot_pos_cmp ) ) detonated_loot_containers.push_back( loot_entt );
    }
    for ( auto loot_entt : detonated_loot_containers )
    {
      get_systems_event_queue().trigger( Events::DestroyLootContainerEvent( loot_entt ) );
    }

    // detonate npc containers - these are activated by proximity so just destroy them
    Utils::Collision::for_each_intersect<Cmp::Npc::Container>( reg(), armed_pos_cmp,
                                                               [&]( entt::entity npc_entity, Cmp::Npc::Container &, Cmp::Position & )
    { Factory::Npc::destroy_npc_container( reg(), npc_entity, reserved_sm ); } );

    // detonate nearby WorldItems - cruel but fair
    Utils::Collision::for_each_intersect<Cmp::WorldItem>( reg(), armed_pos_cmp,
                                                          [&]( entt::entity item_entt, Cmp::WorldItem &item_cmp, Cmp::Position &item_pos_cmp )
    {
      if ( item_entt == armed_entt ) return;
      // e.g. loot dropped by a grave this blast exhumed
      if ( reg().any_of<Cmp::Inventory::BlastProof>( item_entt ) ) return;
      if ( item_cmp.item_type == "item.shovel" )
      {
        auto dmg = Sys::PersistSystem::get<Cmp::Persist::BombDamage>( reg() ).get_value();
        get_systems_event_queue().trigger( Events::UpdateDmgEvent( Utils::Player::get_inventory_entt( reg() ), dmg ) );
      }
      else if ( item_cmp.item_type == "item.exitkey" or item_cmp.item_type == "item.cryptkey" ) { return; }
      else if ( item_cmp.item_type == "item.bomb" )
      {
        // process other bombs lying around - chain reaction!
        // Skip if this carryitem was already armed (already processed or being processed)
        if ( reg().any_of<Cmp::Armed>( item_entt ) )
        {
          get_systems_event_queue().trigger( Events::DestroyItemEvent( item_entt ) );
          return;
        }

        // IMMEDIATELY arm it to prevent other recursive calls from processing it
        Factory::Bomb::create_armed( reg(), item_entt, Cmp::Armed::EpiCenter::YES, 0, item_pos_cmp.position.y - 64 );
        arm_entt( item_entt );
        SPDLOG_INFO( "Chain reaction triggered for bomb entity {} ", static_cast<int>( item_entt ) );
      }
      else { get_systems_event_queue().trigger( Events::DestroyItemEvent( item_entt ) ); }
    } );

    // Check player explosion damage
    auto player_view = reg().view<Cmp::Player::Character, Cmp::PlayerStats, Cmp::Player::Mortality, const Cmp::Position>();
    for ( auto [pc_entt, pc_cmp, player_stats_cmp, pc_mort_cmp, pc_pos_cmp] : player_view.each() )
    {
      if ( pc_pos_cmp.findIntersection( armed_pos_cmp ) )
      {
        auto &bomb_damage = Sys::PersistSystem::get<Cmp::Persist::BombDamage>( reg() );
        // pc_health_cmp.health -= bomb_damage.get_value();
        player_stats_cmp.apply( { Cmp::Stats::Health{ -bomb_damage.get_value() }, {}, {}, {}, {}, {}, {} } );
        if ( player_stats_cmp.health() <= 0 )
        {
          get_systems_event_queue().enqueue(
              Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::EXPLODING, Utils::Player::get_position( reg() ) ) );
        }
      }
    }

    // Check if NPC was killed by explosion.
    // Take a snapshot of the NPC view to avoid updating it live (undefined behaviour).
    std::vector<entt::entity> exploded_npcs;
    for ( auto [npc_entt, npc_cmp, npc_pos_cmp, npc_anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::AnimData>().each() )
    {
      if ( npc_anim_cmp.m_sprite_type.contains( "sprite.ghost" ) ) continue;
      // crows can't be killed, see NpcSystem::on_destroy_npc_event
      if ( reg().any_of<Cmp::Npc::Crow>( npc_entt ) ) continue;
      if ( npc_pos_cmp.findIntersection( armed_pos_cmp ) ) exploded_npcs.push_back( npc_entt );
    }

    for ( entt::entity npc_entt : exploded_npcs )
    {
      auto *npc_pos_cmp = reg().try_get<Cmp::Position>( npc_entt );
      if ( not npc_pos_cmp ) continue; // already processed (e.g. removed by an earlier entity's side effects)

      SPDLOG_INFO( "NPC entity {} exploded at {},{}", static_cast<int>( npc_entt ), npc_pos_cmp->position.x, npc_pos_cmp->position.y );
      get_systems_event_queue().trigger( Events::DestroyNpcEvent( npc_entt, Sys::SpriteKey( "sprite.death.anim.explosion" ) ) );
    }

    // play sound effect if this armed component is epicenter
    if ( is_epicenter ) { m_sound_bank.get_effect( "bomb_detonate" ).play(); }

    // check if we have any epicenter armed components before stopping the fuse sound
    bool remaining_epicenter_bombs = false;
    for ( auto [other_armed_entity, other_armed_cmp] : reg().view<Cmp::Armed>().each() )
    {
      if ( other_armed_cmp.m_epicenter == Cmp::Armed::EpiCenter::YES )
      {
        remaining_epicenter_bombs = true;
        break; // we dont care how many
      }
    }
    if ( not remaining_epicenter_bombs ) m_sound_bank.get_effect( "bomb_fuse" ).stop();

    // finally delete the armed component
    Factory::Bomb::destroy_armed( reg(), armed_entt );

    // Replace the armed position with a detonated sprite for visual effect - make sure its z-order is furthest back,
    // but skip if a detonated entity already occupies this position (e.g. overlapping blast patterns)
    bool already_detonated = false;
    Utils::Collision::for_each_intersect<Cmp::DestroyedObstacle>( reg(), armed_pos_cmp, [&]( entt::entity, Cmp::DestroyedObstacle &, Cmp::Position & )
    { already_detonated = true; } );
    if ( not already_detonated ) { Factory::Bomb::add_detonated( reg(), armed_entt, armed_pos_cmp ); }
  }

  auto remaining_armed_view = reg().view<Cmp::Armed>();
  if ( remaining_armed_view->empty() )
  {
    Utils::Player::get_global_bomb_flash_timer( reg() ) = sf::Time::Zero;
    // the explosion sequence is over, so items it spared can be blown up by the next one
    reg().clear<Cmp::Inventory::BlastProof>();
  }
}

void BombSystem::place_concentric_bomb_pattern( const entt::entity &epicenter_entity, const int blast_radius )
{
  constexpr float kZOrderOffset = 64.f;

  // Validate epicenter entity
  if ( not reg().valid( epicenter_entity ) ) return;

  // Skip if this entity is already armed (prevents re-processing)
  if ( reg().any_of<Cmp::Armed>( epicenter_entity ) ) return;

  auto grid_pos_opt = Utils::get_grid_position<int>( reg(), epicenter_entity );
  if ( not grid_pos_opt.has_value() ) return;
  sf::Vector2i centerTile = grid_pos_opt.value();

  // Mark epicenter as armed FIRST before any recursive processing
  int sequence_counter = 0;
  Factory::Bomb::create_armed( reg(), epicenter_entity, Cmp::Armed::EpiCenter::YES, sequence_counter++, centerTile.y + kZOrderOffset );
  reg().get<Cmp::Armed>( epicenter_entity ).m_blast_radius = blast_radius;

  // We dont detonate reserved positions (or the exit) so dont arm them, but they still get a visual-only
  // Cmp::BlastMarker so the blast pattern has no holes.
  // Also exclude NPCs since they're handled separately and may be missing Position component during death animation.
  // Obstacle caps are excluded too: a cap is only ever removed with its obstacle main (see
  // Factory::Obstacle::remove_obstacle()), so an obstacle is hit only when its main's cell is in the blast. Arming a cap
  // on its own would strip its sprite and leave the main standing, with the scorched cap still selecting it for digging.
  // The cap's cell is still armed and scorched through its own floor entity.
  auto all_obstacle_view = reg().view<Cmp::Armable, Cmp::Position>( exclude<Cmp::Npc::NPC, Cmp::ObstacleCap> );
  auto reserved_sm = m_reserved_sm.lock();
  auto hazard_sm = m_hazard_sm.lock();

  // cells already showing a marker (e.g. from an overlapping blast), so each cell gets at most one
  std::set<std::pair<int, int>> marked_cells;
  // z-order for each new marker cell, see Cmp::BlastMarker
  std::map<std::pair<int, int>, float> marker_zorders;
  constexpr float kBlastMarkerZOffset = 0.5f;
  for ( auto marker_entt : reg().view<Cmp::BlastMarker, Cmp::Position>() )
  {
    if ( auto cell = Utils::get_grid_position<int>( reg(), marker_entt ) ) marked_cells.emplace( cell->x, cell->y );
  }

  // Bucket every candidate entity by its layer (Chebyshev distance from the epicenter) in a single pass,
  // rather than re-scanning and re-computing distances once per layer. entt::null marks a cell that only gets a marker.
  std::vector<std::vector<std::pair<entt::entity, sf::Vector2i>>> entities_by_layer( static_cast<std::size_t>( std::max( blast_radius, 0 ) ) + 1 );

  const auto bucket_candidate = [&]( entt::entity destructable_entity, const Cmp::Position &destructable_pos )
  {
    if ( destructable_entity == epicenter_entity || reg().any_of<Cmp::Armed>( destructable_entity ) ) return;

    sf::Vector2i grid_position = Utils::get_grid_position<int>( reg(), destructable_entity ).value();
    int distance_from_center = Utils::Maths::getChebyshevDistance( grid_position, centerTile );

    if ( distance_from_center < 1 or distance_from_center > blast_radius ) return;

    const auto reserved_entities = reserved_sm ? reserved_sm->at( destructable_pos ) : std::vector<entt::entity>{};
    // structures, hazard cells and the exit only get a marker - they're never armed
    const bool is_hazard_cell = hazard_sm and not hazard_sm->at( destructable_pos ).empty();
    if ( not reserved_entities.empty() or is_hazard_cell or reg().all_of<Cmp::Exit>( destructable_entity ) )
    {
      const std::pair<int, int> cell{ grid_position.x, grid_position.y };
      if ( marked_cells.emplace( cell ).second )
      {
        entities_by_layer[static_cast<std::size_t>( distance_from_center )].emplace_back( entt::null, grid_position );
      }
      // draw the marker just above the highest sprite on the cell, so the grave/item/exit can't hide it
      float &marker_z = marker_zorders.try_emplace( cell, destructable_pos.position.y ).first->second;
      auto raise_to = [&]( entt::entity e )
      {
        if ( const auto *z_cmp = reg().try_get<Cmp::ZOrderValue>( e ) ) marker_z = std::max( marker_z, z_cmp->get() + kBlastMarkerZOffset );
      };
      raise_to( destructable_entity );
      for ( entt::entity reserved_entt : reserved_entities )
        raise_to( reserved_entt );
      return;
    }

    if ( reg().any_of<Cmp::LootContainer>( destructable_entity ) )
    {
      SPDLOG_DEBUG( "Arming loot container entity {}", static_cast<int>( destructable_entity ) );
    }
    entities_by_layer[static_cast<std::size_t>( distance_from_center )].emplace_back( destructable_entity, grid_position );
  };

  if ( auto world_tile_sm = m_world_tile_sm.lock() )
  {
    // only the world tiles inside the blast square, instead of every Armable entity in the scene. The Armable
    // entities that aren't world tiles (multiblock segments) sit on reserved cells, whose floor tile already takes the
    // marker path above; NPCs are excluded anyway.
    const sf::Vector2f epicenter_px( static_cast<float>( centerTile.x ) * Constants::kGridSizePxF.x,
                                     static_cast<float>( centerTile.y ) * Constants::kGridSizePxF.y );
    const sf::FloatRect blast_bounds( epicenter_px - Constants::kGridSizePxF * static_cast<float>( blast_radius ),
                                      Constants::kGridSizePxF * static_cast<float>( 2 * blast_radius + 1 ) );
    for ( auto destructable_entity : world_tile_sm->query_rect( blast_bounds ) )
    {
      if ( not reg().valid( destructable_entity ) or not all_obstacle_view.contains( destructable_entity ) ) continue;
      bucket_candidate( destructable_entity, all_obstacle_view.get<Cmp::Position>( destructable_entity ) );
    }
  }
  else
  {
    for ( auto [destructable_entity, destructable_cmp, destructable_pos] : all_obstacle_view.each() )
    {
      bucket_candidate( destructable_entity, destructable_pos );
    }
  }

  // For each layer from 1 to BLAST_RADIUS, sort clockwise and arm
  for ( int layer = 1; layer <= blast_radius; layer++ )
  {
    auto &layer_entities = entities_by_layer[static_cast<std::size_t>( layer )];
    SPDLOG_DEBUG( "Layer {}: Found {} entities to arm", layer, layer_entities.size() );

    // clang-format off
    // Sort entities in clockwise order
    std::ranges::sort( layer_entities,
      [centerTile]( const auto &a, const auto &b )
      {
        // Calculate angles from center to points
        float angleA = std::atan2( a.second.y - centerTile.y, a.second.x - centerTile.x );
        float angleB = std::atan2( b.second.y - centerTile.y, b.second.x - centerTile.x );
        return angleA < angleB;
      } );
    // clang-format on

    // Arm each entity in the layer in clockwise order
    for ( const auto &[entity, pos] : layer_entities )
    {
      if ( entity == entt::null )
      {
        // visual-only marker over a cell we must not detonate
        auto marker_entt = reg().create();
        reg().emplace<Cmp::Position>( marker_entt, Constants::kGridSizePxF.componentWiseMul( sf::Vector2f( pos ) ), Constants::kGridSizePxF );
        Factory::Bomb::create_armed( reg(), marker_entt, Cmp::Armed::EpiCenter::NO, sequence_counter++, centerTile.y + kZOrderOffset );
        // create_armed() always adds this, but a marker must not affect pathing or placement checks
        reg().remove<Cmp::Npc::NoPathFinding>( marker_entt );
        reg().emplace<Cmp::BlastMarker>( marker_entt );
        // reserved cells have their own colours, so they read as "in the blast, but not destroyed"
        auto &marker_armed_cmp = reg().get<Cmp::Armed>( marker_entt );
        marker_armed_cmp.m_armed_color_border = Sys::PersistSystem::get<Cmp::Persist::ReservedArmedBlockColourBorder>( reg() );
        marker_armed_cmp.m_armed_color_fill = Sys::PersistSystem::get<Cmp::Persist::ReservedArmedBlockColourFill>( reg() );
        // no sprite of its own, so it is queued at this z-order and drawn by RenderGameSystem::render_zorder_queue()
        reg().emplace_or_replace<Cmp::ZOrderValue>( marker_entt, marker_zorders.at( { pos.x, pos.y } ) );
        continue;
      }
      Factory::Bomb::create_armed( reg(), entity, Cmp::Armed::EpiCenter::NO, sequence_counter++, centerTile.y + kZOrderOffset );
    }
  }
}

void BombSystem::on_bomb_event( const Events::PlayerActionEvent &event )
{
  if ( event.action == Events::PlayerActionEvent::GameActions::PLACE_BOMB ) { arm_player_bomb(); }
  else if ( event.action == Events::PlayerActionEvent::GameActions::TRIGGER_BOMB ) { arm_grave_bomb( event.m_entt ); }
}

void BombSystem::arm_grave_bomb( entt::entity source_entt )
{
  m_sound_bank.get_effect( "bomb_fuse" ).play();
  // centre on the grave that set it off when there is one (e.g. exhumed by another blast), else on the player
  const auto *source_pos_cmp = reg().valid( source_entt ) ? reg().try_get<Cmp::Position>( source_entt ) : nullptr;
  auto realigned_epicenter_pos = Utils::snap_to_grid( source_pos_cmp ? *source_pos_cmp : Utils::Player::get_position( reg() ) );
  auto new_bomb_entt = reg().create();
  reg().emplace_or_replace<Cmp::Position>( new_bomb_entt, realigned_epicenter_pos.position, realigned_epicenter_pos.size );
  place_concentric_bomb_pattern( new_bomb_entt, grave_blast_radius() );
  Utils::Player::get_global_bomb_flash_timer( reg() ) = sf::Time::Zero;
}

void BombSystem::arm_player_bomb()
{
  auto player_pos = Utils::Player::get_position( reg() );

  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type != "item.bomb" ) return;

  // caps share a cell with the floor entity beneath them, so skip them or one bomb would plant two epicentres
  auto destructable_view = reg().view<Cmp::Armable, Cmp::Position>( exclude<Cmp::ObstacleCap> );
  // with the world-tile grid, only the cells around the player; otherwise every Armable entity
  std::vector<entt::entity> nearby_entts;
  auto world_tile_sm = m_world_tile_sm.lock();
  if ( world_tile_sm ) { nearby_entts = world_tile_sm->neighbours( player_pos ); }
  else { nearby_entts.assign( destructable_view.begin(), destructable_view.end() ); }
  for ( auto destructable_entity : nearby_entts )
  {
    if ( not reg().valid( destructable_entity ) or not destructable_view.contains( destructable_entity ) ) continue;
    const auto &destructable_pos_cmp = destructable_view.get<Cmp::Position>( destructable_entity );
    // make a copy and reduce/center the player hitbox to avoid arming a neighbouring location
    auto player_hitbox = sf::FloatRect( player_pos );
    player_hitbox.size.x /= 2.f;
    player_hitbox.size.y /= 2.f;
    player_hitbox.position.x += 4.f;
    player_hitbox.position.y += 4.f;

    // are we standing on a destructable tile?
    if ( player_hitbox.findIntersection( destructable_pos_cmp ) )
    {
      m_sound_bank.get_effect( "bomb_fuse" ).play();

      auto armed_epicenter_entity = reg().create();
      auto realigned_epicenter_pos = Utils::snap_to_grid( destructable_pos_cmp );
      reg().emplace<Cmp::Position>( armed_epicenter_entity, realigned_epicenter_pos.position, realigned_epicenter_pos.size );
      place_concentric_bomb_pattern( armed_epicenter_entity, player_blast_radius() );
      Factory::Player::destroy_inventory( reg(), "item.bomb" );
      Utils::Player::get_global_bomb_flash_timer( reg() ) = sf::Time::Zero;
      // one bomb, one epicentre; arming also created entities, so don't keep iterating the view
      break;
    }
  }
}

void BombSystem::arm_entt( entt::entity target_entt )
{
  // then use the candidate entity to place the booby trap bomb
  if ( target_entt != entt::null )
  {
    m_sound_bank.get_effect( "bomb_fuse" ).play();

    place_concentric_bomb_pattern( target_entt, player_blast_radius() );
  }
}

int BombSystem::player_blast_radius() { return std::max( 1, Utils::Player::get_stats( reg() ).luck() / kLuckPerBlastTile ); }

int BombSystem::grave_blast_radius()
{
  return std::max( kGraveBlastMinRadius, kGraveBlastMaxRadius - ( Utils::Player::get_stats( reg() ).luck() / kGraveLuckPerBlastTile ) );
}

} // namespace Game::Sys