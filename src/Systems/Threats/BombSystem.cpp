#include <Audio/SoundBank.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/Armed.hpp>
#include <Components/Crypt/BuildingSegment.hpp>
#include <Components/DeathPosition.hpp>
#include <Components/DestroyedObstacle.hpp>
#include <Components/Exit.hpp>
#include <Components/Grave/Segment.hpp>
#include <Components/Inventory/Explosive.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LootContainer.hpp>
#include <Components/Npc/Container.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Persistent/ArmedOffDelay.hpp>
#include <Components/Persistent/BombDamage.hpp>
#include <Components/Player/BlastRadius.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/PauseClocksEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Events/ResumeClocksEvent.hpp>
#include <Factory/BombFactory.hpp>
#include <Factory/LootFactory.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Factory/SpriteFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Threats/BombSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>

#include <algorithm>
#include <optional>
#include <spdlog/spdlog.h>

namespace Game::Sys
{
using entt::exclude;

BombSystem::BombSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank )
{
  // The entt::dispatcher is independent of the registry, so it is safe to bind event handlers in
  // the constructor
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&Sys::BombSystem::on_bomb_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::PauseClocksEvent>().connect<&Sys::BombSystem::on_pause>( this );
  std::ignore = get_systems_event_queue().sink<Events::ResumeClocksEvent>().connect<&Sys::BombSystem::on_resume>( this );
  SPDLOG_DEBUG( "BombSystem initialized" );
}

void BombSystem::on_pause()
{
  if ( m_sound_bank.get_effect( "bomb_fuse" ).getStatus() == sf::Sound::Status::Playing ) m_sound_bank.get_effect( "bomb_fuse" ).pause();
  if ( m_sound_bank.get_effect( "bomb_detonate" ).getStatus() == sf::Sound::Status::Playing ) m_sound_bank.get_effect( "bomb_detonate" ).pause();
  auto armed_view = reg().view<Cmp::Armed>();
  for ( auto [entt, armed_cmp] : armed_view.each() )
  {
    if ( armed_cmp.m_fuse_delay_clock.isRunning() ) armed_cmp.m_fuse_delay_clock.stop();
    if ( armed_cmp.m_warning_delay_clock.isRunning() ) armed_cmp.m_warning_delay_clock.stop();
  }
}
void BombSystem::on_resume()
{
  if ( m_sound_bank.get_effect( "bomb_fuse" ).getStatus() == sf::Sound::Status::Paused ) m_sound_bank.get_effect( "bomb_fuse" ).play();
  if ( m_sound_bank.get_effect( "bomb_detonate" ).getStatus() == sf::Sound::Status::Paused ) m_sound_bank.get_effect( "bomb_detonate" ).play();
  auto armed_view = reg().view<Cmp::Armed>();
  for ( auto [entt, armed_cmp] : armed_view.each() )
  {
    if ( not armed_cmp.m_fuse_delay_clock.isRunning() ) armed_cmp.m_fuse_delay_clock.start();
    if ( not armed_cmp.m_warning_delay_clock.isRunning() ) armed_cmp.m_warning_delay_clock.start();
  }
}

void BombSystem::on_bomb_event( const Events::PlayerActionEvent &event )
{
  if ( event.action == Events::PlayerActionEvent::GameActions::PLACE_BOMB ) { arm_player_bomb(); }
  else if ( event.action == Events::PlayerActionEvent::GameActions::TRIGGER_BOMB ) { arm_grave_bomb(); }
}

void BombSystem::arm_grave_bomb()
{
  m_sound_bank.get_effect( "bomb_fuse" ).play();
  auto new_bomb_entt = reg().create();
  auto realigned_epicenter_pos = Utils::snap_to_grid( Utils::Player::get_position( reg() ) );
  reg().emplace_or_replace<Cmp::Position>( new_bomb_entt, realigned_epicenter_pos.position, realigned_epicenter_pos.size );
  place_concentric_bomb_pattern( new_bomb_entt, Utils::Player::get_blast_radius( reg() ).value );
  Utils::Player::get_global_bomb_flash_clk( reg() ).restart();
}

void BombSystem::arm_entt( entt::entity target_entt )
{
  // then use the candidate entity to place the booby trap bomb
  if ( target_entt != entt::null )
  {
    m_sound_bank.get_effect( "bomb_fuse" ).play();

    place_concentric_bomb_pattern( target_entt, Utils::Player::get_blast_radius( reg() ).value );
  }
}

void BombSystem::arm_player_bomb()
{
  auto player_pos = Utils::Player::get_position( reg() );

  auto destructable_view = reg().view<Cmp::Armable, Cmp::Position>();
  for ( auto [destructable_entity, destructable_cmp, destructable_pos_cmp] : destructable_view.each() )
  {

    auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
    if ( inventory_type != "item.bomb" ) return;

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
      place_concentric_bomb_pattern( armed_epicenter_entity, Utils::Player::get_blast_radius( reg() ).value );
      Factory::Player::destroy_inventory( reg(), "item.bomb" );
      Utils::Player::get_global_bomb_flash_clk( reg() ).restart();
    }
  }
}

void BombSystem::place_concentric_bomb_pattern( const entt::entity &epicenter_entity, const int blast_radius, int depth )
{
  constexpr float kZOrderOffset = 64.f;

  constexpr int kMaxRecursionDepth = 10;
  if ( depth >= kMaxRecursionDepth ) return;

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

  // We dont detonate reserved positions so dont arm them in the first place
  // Also exclude NPCs since they're handled separately and may be missing Position component during death animation
  auto all_obstacle_view = reg().view<Cmp::Armable, Cmp::Position>( exclude<Cmp::Npc::NPC, Cmp::Exit> );
  auto reserved_sm = m_reserved_sm.lock();

  // For each layer from 1 to BLAST_RADIUS
  for ( int layer = 1; layer <= blast_radius; layer++ )
  {
    std::vector<std::pair<entt::entity, sf::Vector2i>> layer_entities;

    // Collect all entities in this layer with their positions
    for ( auto [destructable_entity, destructable_cmp, destructable_pos] : all_obstacle_view.each() )
    {
      if ( destructable_entity == epicenter_entity || reg().any_of<Cmp::Armed>( destructable_entity ) ) continue;
      if ( reserved_sm && not reserved_sm->at( destructable_pos ).empty() ) continue;

      sf::Vector2i grid_position = Utils::get_grid_position<int>( reg(), destructable_entity ).value();
      int distance_from_center = Utils::Maths::getChebyshevDistance( grid_position, centerTile );

      if ( distance_from_center == layer )
      {
        if ( reg().any_of<Cmp::LootContainer>( destructable_entity ) )
        {
          SPDLOG_DEBUG( "Arming loot container entity {}", static_cast<int>( destructable_entity ) );
        }
        layer_entities.emplace_back( destructable_entity, grid_position );
      }
    }
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
      Factory::Bomb::create_armed( reg(), entity, Cmp::Armed::EpiCenter::NO, sequence_counter++, centerTile.y + kZOrderOffset );
    }
  }
}

void BombSystem::update()
{

  PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = m_npc_navmesh.lock();
  if ( not pathfinding_navmesh )
  {
    SPDLOG_WARN( "Unable to lock weakptr: pathfinding_navmesh" );
    return;
  }

  PathFinding::SpatialHashGridSharedPtr player_navmesh = m_player_navmesh.lock();
  if ( not player_navmesh )
  {
    SPDLOG_WARN( "Unable to lock weakptr: player_navmesh" );
    return;
  }

  auto reserved_sm = m_reserved_sm.lock();
  auto armed_view = reg().view<Cmp::Armed, Cmp::Position>();
  for ( auto [armed_entt, armed_cmp, armed_pos_cmp] : armed_view.each() )
  {
    if ( armed_cmp.getElapsedFuseTime() < armed_cmp.m_fuse_delay ) continue;

    // detonate obstacles - remove all traces of obstacle
    auto obstacle_view = reg().view<Cmp::Obstacle, Cmp::Position>();
    for ( auto [obst_entity, obst_cmp, obst_pos_cmp] : obstacle_view.each() )
    {
      if ( not obst_pos_cmp.findIntersection( armed_pos_cmp ) ) continue;
      if ( reserved_sm && not reserved_sm->at( obst_pos_cmp ).empty() ) continue;
      Factory::Obstacle::remove_obstacle( reg(), obst_entity, Factory::Obstacle::DeleteExtras::Yes, reserved_sm );
      pathfinding_navmesh->insert( obst_entity, obst_pos_cmp );
      player_navmesh->insert( obst_entity, obst_pos_cmp );
      if ( auto ghost_navmesh = m_ghost_navmesh.lock() ) { ghost_navmesh->insert( obst_entity, obst_pos_cmp ); }
    }

    // detonate loot containers - component removal is handled by LootSystem
    auto loot_container_view = reg().view<Cmp::LootContainer, Cmp::Position>();
    for ( auto [loot_entt, loot_cmp, loot_pos_cmp] : loot_container_view.each() )
    {
      if ( not loot_pos_cmp.findIntersection( armed_pos_cmp ) ) continue;

      if ( loot_entt != entt::null ) { m_sound_bank.get_effect( "break_pot" ).play(); }

      Factory::Loot::destroy_loot_container( reg(), loot_entt, reserved_sm );
    }

    // detonate npc containers - these are activated by proximity so just destroy them
    auto npc_container_view = reg().view<Cmp::Npc::Container, Cmp::Position>();
    for ( auto [npc_entity, npc_cmp, npc_pos_cmp] : npc_container_view.each() )
    {
      if ( not npc_pos_cmp.findIntersection( armed_pos_cmp ) ) continue;
      Factory::Npc::destroy_npc_container( reg(), npc_entity, reserved_sm );
    }

    // detonate nearby carryitems - cruel but fair
    auto carryitem_view = reg().view<Cmp::WorldItem, Cmp::Position>();
    for ( auto [item_entt, item_cmp, item_pos_cmp] : carryitem_view.each() )
    {
      if ( item_entt == armed_entt ) continue;
      if ( not item_pos_cmp.findIntersection( armed_pos_cmp ) ) continue;
      if ( item_cmp.item_type == "item.pickaxe" or item_cmp.item_type == "item.axe" or item_cmp.item_type == "item.shovel" )
      {
        Utils::Player::reduce_inventory_wear_level( reg(), Sys::PersistSystem::get<Cmp::Persist::BombDamage>( reg() ).get_value() );
      }
      else if ( item_cmp.item_type == "item.bomb" )
      {
        // process other explosives lying around - chain reaction!
        auto *explosive_cmp = reg().try_get<Cmp::Explosive>( item_entt );
        if ( not explosive_cmp ) continue;
        SPDLOG_DEBUG( "Found explosive component {}", static_cast<int>( carryitem_entt ) );

        // Skip if this carryitem was already armed (already processed or being processed)
        if ( explosive_cmp->armed and armed_pos_cmp.findIntersection( item_pos_cmp ) )
        {
          if ( reg().valid( item_entt ) ) { reg().destroy( item_entt ); }
        }
        SPDLOG_DEBUG( "Explosive candidate not already armed {}", static_cast<int>( carryitem_entt ) );

        // Check if this bomb is within the blast radius of THIS explosion (not just any armed position)
        if ( not armed_pos_cmp.findIntersection( item_pos_cmp ) ) continue;
        SPDLOG_DEBUG( "Found explosive candidate {}", static_cast<int>( carryitem_entt ) );

        // IMMEDIATELY mark as armed to prevent other recursive calls from processing it
        explosive_cmp->armed = true;
        Factory::Bomb::create_armed( reg(), item_entt, Cmp::Armed::EpiCenter::YES, 0, item_pos_cmp.position.y - 64 );
        arm_entt( item_entt );
        SPDLOG_INFO( "Chain reaction triggered for bomb entity {} ", static_cast<int>( item_entt ) );
      }
      else
      {
        if ( reg().valid( item_entt ) ) { reg().destroy( item_entt ); }
      }
    }

    // Check player explosion damage
    auto player_view = reg().view<Cmp::Player::Character, Cmp::PlayerStats, Cmp::Player::Mortality, Cmp::Position>();
    for ( auto [pc_entt, pc_cmp, player_stats_cmp, pc_mort_cmp, pc_pos_cmp] : player_view.each() )
    {
      if ( pc_pos_cmp.findIntersection( armed_pos_cmp ) )
      {
        auto &bomb_damage = Sys::PersistSystem::get<Cmp::Persist::BombDamage>( reg() );
        // pc_health_cmp.health -= bomb_damage.get_value();
        player_stats_cmp.apply_modifiers( { Cmp::Stats::Health{ -bomb_damage.get_value() }, {}, {}, {}, {}, {}, {} } );
        if ( player_stats_cmp.health() <= 0 )
        {
          get_systems_event_queue().enqueue(
              Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::EXPLODING, Utils::Player::get_position( reg() ) ) );
        }
      }
    }

    // Check if NPC was killed by explosion
    for ( auto [npc_entt, npc_cmp, npc_pos_cmp, npc_anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::AnimData>().each() )
    {
      if ( npc_anim_cmp.m_sprite_type.contains( "sprite.ghost" ) ) continue;
      // notify npc system of death
      if ( npc_pos_cmp.findIntersection( armed_pos_cmp ) )
      {
        Factory::Npc::create_npc_death_anim( reg(), npc_pos_cmp, "sprite.death.anim.explosion" );

        SPDLOG_INFO( "NPC entity {} exploded at {},{}", static_cast<int>( npc_entt ), npc_pos_cmp.position.x, npc_pos_cmp.position.y );
        Factory::Npc::destroy_npc( reg(), npc_entt );

        auto [sprite_type, sprite_index] = m_sprite_factory.get_random_type_and_texture_index(
            std::vector<std::string>{ "sprite.graveyard.loot.health", "sprite.graveyard.loot.blast", "sprite.graveyard.loot.repair" } );

        Cmp::RandomInt do_drop( 0, 2 ); // 1 in 3 chance of no drop
        if ( do_drop.gen() == 0 )
        {
          // clang-format off
          auto dropped_loot_entt = Factory::Loot::create_loot_drop(
            reg(),
            Cmp::AnimData( Cmp::AnimData::Config{ .sprite_type = sprite_type, .frame_index_offset = sprite_index} ),
            sf::FloatRect{ npc_pos_cmp.position, npc_pos_cmp.size },
            Factory::IncludePack<>{},
            Factory::ExcludePack<Cmp::Player::Character>{}, Factory::ExcludePack<>{},
            /*zorder_offset=*/-8.f, reserved_sm.get() );
          // clang-format on

          if ( dropped_loot_entt != entt::null )
          {
            SPDLOG_INFO( "NPC dropped loot." );
            m_sound_bank.get_effect( "drop_loot" ).play();
          }
        }
      }
    }

    // play sound effect if this armed component is epicenter
    if ( armed_cmp.m_epicenter == Cmp::Armed::EpiCenter::YES ) { m_sound_bank.get_effect( "bomb_detonate" ).play(); }

    // check if we have any epicenter armed components before stopping the fuse sound
    bool remaining_epicenter_bombs = false;
    for ( auto [armed_entity, armed_cmp] : reg().view<Cmp::Armed>().each() )
    {
      if ( armed_cmp.m_epicenter == Cmp::Armed::EpiCenter::YES )
      {
        remaining_epicenter_bombs = true;
        break; // we dont care how many
      }
    }
    if ( not remaining_epicenter_bombs ) m_sound_bank.get_effect( "bomb_fuse" ).stop();

    // finally delete the armed component
    Factory::Bomb::destroy_armed( reg(), armed_entt );

    // Replace the armed position with a detonated sprite for visual effect - make sure its z-order is furthest back
    Factory::Bomb::add_detonated( reg(), armed_entt, armed_pos_cmp );
  }

  auto remaining_armed_view = reg().view<Cmp::Armed>();
  if ( remaining_armed_view->empty() ) { Utils::Player::get_global_bomb_flash_clk( reg() ).reset(); }
}

} // namespace Game::Sys