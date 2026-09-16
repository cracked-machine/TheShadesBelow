
#include <Audio/SoundBank.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/AnimData.hpp>
#include <Components/Crypt/BuildingSegment.hpp>
#include <Components/Direction.hpp>
#include <Components/Exit.hpp>
#include <Components/Grave/ExitSegment.hpp>
#include <Components/Grave/Segment.hpp>
#include <Components/Hazard/FieldCell.hpp>
#include <Components/LerpPosition.hpp>
#include <Components/LootContainer.hpp>
#include <Components/Npc/Container.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Persistent/WormholeSeed.hpp>
#include <Components/Player/Character.hpp>
#include <Components/RectBounds.hpp>
#include <Components/UUID.hpp>
#include <Components/Wall.hpp>
#include <Components/Wormhole/Jump.hpp>
#include <Components/Wormhole/MultiBlock.hpp>
#include <Components/Wormhole/Singularity.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/PauseClocksEvent.hpp>
#include <Events/ResumeClocksEvent.hpp>
#include <Factory/LootFactory.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Threats/WormholeSystem.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>

#include <SFML/System/Vector2.hpp>
#include <algorithm>

namespace Game::Sys
{

WormholeSystem::WormholeSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank )
{

  // The entt::dispatcher is independent of the registry, so it is safe to bind event handlers in the constructor
  get_systems_event_queue().sink<Events::PauseClocksEvent>().connect<&Sys::WormholeSystem::on_pause>( this );
  get_systems_event_queue().sink<Events::ResumeClocksEvent>().connect<&Sys::WormholeSystem::on_resume>( this );

  SPDLOG_DEBUG( "WormholeSystem initialized" );
}

void WormholeSystem::check_player_wormhole_collision()
{
  auto wormhole_view = reg().view<Cmp::Wormhole::Singularity, Cmp::Position>();
  auto all_actors_view = reg().view<Cmp::Direction, Cmp::Position>();

  // First, check for any entities with Jump that are NOT colliding
  auto jump_view = reg().view<Cmp::Wormhole::Jump>();
  for ( auto [entity, jump_cmp] : jump_view.each() )
  {
    bool still_colliding = false;

    auto *jump_pos_cmp = reg().try_get<Cmp::Position>( entity );
    if ( not jump_pos_cmp )
    {
      SPDLOG_DEBUG( "Entity {} has Jump but NO Position component - removing jump", static_cast<uint32_t>( entity ) );
      reg().remove<Cmp::Wormhole::Jump>( entity );
      continue;
    }

    for ( auto [wormhole_entity, wormhole_cmp, wh_pos_cmp] : wormhole_view.each() )
    {
      auto wh_hitbox_redux = Cmp::RectBounds::scaled( wh_pos_cmp.position, wh_pos_cmp.size, 1.f );
      if ( jump_pos_cmp && jump_pos_cmp->findIntersection( wh_hitbox_redux.getBounds() ) )
      {
        still_colliding = true;
        break;
      }
    }

    if ( !still_colliding )
    {
      SPDLOG_WARN( "Entity {} has Jump but is NO LONGER colliding - removing jump component", static_cast<uint32_t>( entity ) );
      reg().remove<Cmp::Wormhole::Jump>( entity );
      m_sound_bank.get_effect( "wormhole_jump" ).stop();
    }
  }

  // Now check for new/ongoing collisions and collect entities ready to teleport
  for ( auto [actor_entity, actor_dir_cmp, actor_pos_cmp] : all_actors_view.each() )
  {
    for ( auto [wormhole_entity, wormhole_cmp, wh_pos_cmp] : wormhole_view.each() )
    {

      auto wh_hitbox_redux = Cmp::RectBounds::scaled( wh_pos_cmp.position, wh_pos_cmp.size, 1.f );
      if ( !actor_pos_cmp.findIntersection( wh_hitbox_redux.getBounds() ) ) continue;

      // Check if jump component already exists
      auto *wh_jump_cmp = reg().try_get<Cmp::Wormhole::Jump>( actor_entity );
      if ( !wh_jump_cmp )
      {
        // First collision - create component
        reg().emplace<Cmp::Wormhole::Jump>( actor_entity );
        SPDLOG_INFO( "Entity {} is jump candidate.", static_cast<uint32_t>( actor_entity ) );
        // restart the jump sfx for each actor processed so that it is heard by the last actor.
        // There is adequate lead time on the sfx (~2secs) to prevent restart stuttering.
        m_sound_bank.get_effect( "wormhole_jump" ).play();
      }
    }
  }

  // Count how many entitiesare ready to teleport
  std::size_t teleport_ready_count = 0;
  for ( auto [entity, jump_cmp] : jump_view.each() )
  {
    // Check if cooldown complete
    float elapsed = jump_cmp.jump_clock.getElapsedTime().asSeconds();
    float cooldown = jump_cmp.jump_cooldown.asSeconds();

    if ( elapsed >= cooldown ) { teleport_ready_count++; }
  }

  // Commence teleportation if all entities are ready to jump
  if ( teleport_ready_count == jump_view.size() && teleport_ready_count > 0 )
  {
    SPDLOG_INFO( "Teleportation commencing. Jump candidates: {}", jump_view.size() );
    for ( auto [entity, jump_cmp] : jump_view.each() )
    {

      // Get random teleported position for this actor entity
      auto [new_spawn_entity, new_spawn_pos_cmp] = find_spawn_location( 0 );
      if ( new_spawn_entity == entt::null )
      {
        SPDLOG_ERROR( "Teleport failed: no valid destination found for entity {}", static_cast<uint32_t>( entity ) );
        continue;
      }

      Factory::Obstacle::remove_obstacle( reg(), new_spawn_entity, Factory::Obstacle::DeleteExtras::Yes, m_reserved_sm.lock() );
      if ( auto teleport_navmesh = m_npc_navmesh.lock() ) teleport_navmesh->insert( new_spawn_entity, new_spawn_pos_cmp );
      // clang-format off
      reg().emplace_or_replace<Cmp::AnimData>( new_spawn_entity, Cmp::AnimData::Config{ .sprite_type = "sprite.graveyard.detonated", .enabled = true });
      // clang-format on
      reg().emplace_or_replace<Cmp::ZOrderValue>( new_spawn_entity, new_spawn_pos_cmp.position.y - 256.f );

      // update the teleported entity's components
      SPDLOG_DEBUG( "Entity {} - TELEPORTING NOW!", static_cast<uint32_t>( entity ) );
      reg().remove<Cmp::LerpPosition>( entity );
      reg().emplace_or_replace<Cmp::Position>( entity, new_spawn_pos_cmp.position, new_spawn_pos_cmp.size );
      reg().remove<Cmp::Wormhole::Jump>( entity );

      SPDLOG_INFO( "Entity {} - TELEPORT to ({}, {}) COMPLETE", static_cast<uint32_t>( entity ), new_spawn_pos_cmp.position.x,
                   new_spawn_pos_cmp.position.y );
    }

    // respawn the wormhole now all entities have teleported
    SPDLOG_INFO( "Teleportation complete. Jump candidates: {}", jump_view.size() );
    despawn_wormhole();
    spawn_wormhole( WormholeSystem::SpawnPhase::Respawn );
  }
}

void WormholeSystem::spawn_wormhole( SpawnPhase phase )
{
  // 1. pick a random position component in the maze, exclude walls, doors, exits, and playable characters
  // 2. get the entity at that position
  unsigned long seed = 0;
  if ( phase == SpawnPhase::InitialSpawn ) seed = Sys::PersistSystem::get<Cmp::Persist::WormholeSeed>( reg() ).get_value();

  auto [spawn_entity, multiblock_pos] = find_spawn_location( seed );
  if ( spawn_entity == entt::null )
  {
    SPDLOG_ERROR( "Failed to find valid wormhole spawn position." );
    return;
  }

  // 3. Create the sprite
  const auto &wormhole_ss = m_sprite_factory.get_spritesheet_by_type( "sprite.graveyard.hazard.wormhole" );
  Cmp::Wormhole::MultiBlock wormhole_block( multiblock_pos.position, wormhole_ss.get_px_size() );

  // clear_footprint destroys every entity occupying the footprint, including spawn_entity itself -
  // its handle is stale from this point on, so the wormhole's MultiBlock entity must be created fresh below.
  clear_footprint( wormhole_block );

  auto reserved_sm = m_reserved_sm.lock();
  auto uuid_cmp = Cmp::UUID::generate();

  // 4. add the wormhole components to a freshly created entity
  sf::Vector2f center_pos = multiblock_pos.position + Constants::kGridSizePxF;
  auto center_entity = reg().create();
  reg().emplace<Cmp::Position>( center_entity, center_pos, Constants::kGridSizePxF );
  reg().emplace<Cmp::Wormhole::Singularity>( center_entity );

  auto multiblock_entity = reg().create();
  reg().emplace<Cmp::Position>( multiblock_entity, multiblock_pos.position, wormhole_ss.get_px_size() );
  reg().emplace<Cmp::Wormhole::MultiBlock>( multiblock_entity, multiblock_pos.position, wormhole_ss.get_px_size() );
  reg().emplace<Cmp::AnimData>( multiblock_entity, Cmp::AnimData::Config{ .sprite_type = "sprite.graveyard.hazard.wormhole", .enabled = true } );
  reg().emplace<Cmp::UUID>( multiblock_entity, uuid_cmp.data );
  reg().emplace<Cmp::ZOrderValue>( multiblock_entity, multiblock_pos.position.y - 16 );

  Factory::Particle::add_wormhole_ps( reg(), "graveyard.wormhole.particles", 1.f, 25.f, uuid_cmp, wormhole_block.getCenter(), 5000.f );

  // reserve both wormhole entities so BombSystem's blast-arming sweep skips them
  if ( reserved_sm )
  {
    reserved_sm->insert( multiblock_entity, multiblock_pos );
    reserved_sm->insert( center_entity, reg().get<Cmp::Position>( center_entity ) );
  }

  SPDLOG_INFO( "Wormhole spawned at position ({}, {}) with zorder: {}", multiblock_pos.position.x, multiblock_pos.position.y,
               multiblock_pos.position.y - multiblock_pos.size.y );
}

void WormholeSystem::on_pause()
{
  if ( m_sound_bank.get_effect( "wormhole_jump" ).getStatus() == sf::Sound::Status::Playing ) m_sound_bank.get_effect( "wormhole_jump" ).pause();

  auto jump_view = reg().view<Cmp::Wormhole::Jump>();
  for ( auto [entity, jump_cmp] : jump_view.each() )
  {
    jump_cmp.jump_clock.stop();
  }
}

void WormholeSystem::on_resume()
{
  if ( m_sound_bank.get_effect( "wormhole_jump" ).getStatus() == sf::Sound::Status::Paused ) m_sound_bank.get_effect( "wormhole_jump" ).play();

  auto jump_view = reg().view<Cmp::Wormhole::Jump>();
  for ( auto [entity, jump_cmp] : jump_view.each() )
  {
    jump_cmp.jump_clock.start();
  }
}

void WormholeSystem::clear_footprint( const sf::FloatRect &bounds )
{
  auto reserved_sm = m_reserved_sm.lock();
  std::vector<std::pair<entt::entity, sf::Vector2f>> kill_list;
  for ( auto [entity, occupant_pos] : reg().view<Cmp::Position>().each() )
  {
    if ( not occupant_pos.findIntersection( bounds ) ) continue;
    SPDLOG_DEBUG( "WH: #{}: {},{}", static_cast<uint32_t>( entity ), occupant_pos.x(), occupant_pos.y() );
    kill_list.emplace_back( entity, occupant_pos.position );
    SPDLOG_DEBUG( "Wormhole: clearing item at ({}, {})", occupant_pos.position.x, occupant_pos.position.y );
  }

  for ( auto [entity, pos] : kill_list )
  {
    if ( not reg().valid( entity ) ) continue;
    Factory::Obstacle::remove_obstacle( reg(), entity, Factory::Obstacle::DeleteExtras::Yes, reserved_sm );
    reg().destroy( entity );
  }

  std::vector<sf::Vector2f> new_list;
  for ( auto [_, pos] : kill_list )
  {
    // only add unique positions
    auto it = std::ranges::find( new_list, pos );
    if ( it == new_list.end() )
    {
      Factory::Obstacle::create_world_pos( reg(), pos );
      new_list.push_back( pos );
    }
  }
}

void WormholeSystem::despawn_wormhole()
{
  auto reserved_sm = m_reserved_sm.lock();

  if ( auto entity = reg().view<Cmp::Wormhole::Singularity>().front(); entity != entt::null )
  {
    if ( auto *pos_cmp = reg().try_get<Cmp::Position>( entity ); pos_cmp && reserved_sm ) { reserved_sm->remove( entity, *pos_cmp ); }
    reg().destroy( entity );
  }

  if ( auto entity = reg().view<Cmp::Wormhole::MultiBlock>().front(); entity != entt::null )
  {
    Cmp::Wormhole::MultiBlock mb_cmp = reg().get<Cmp::Wormhole::MultiBlock>( entity );
    clear_footprint( mb_cmp );
    if ( reg().valid( entity ) )
    {
      if ( auto *pos_cmp = reg().try_get<Cmp::Position>( entity ); pos_cmp && reserved_sm ) { reserved_sm->remove( entity, *pos_cmp ); }
      reg().destroy( entity );
    }
  }

  for ( auto [ps_entt, ps_cmp, ps_uuid_cmp] : reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
  {
    if ( ps_cmp.sprite->get_tag() == "graveyard.wormhole.particles" ) reg().destroy( ps_entt );
  }
}

std::pair<entt::entity, Cmp::Position> WormholeSystem::find_spawn_location( unsigned long initial_rng_seed )
{
  constexpr int kMaxAttempts = 1000;
  int attempts = 0;
  unsigned long new_rng_seed = initial_rng_seed;

  while ( attempts < kMaxAttempts )
  {
    auto [random_entity, random_pos] = Utils::Rnd::get_random_position(
        reg(), Utils::Rnd::IncludePack<Cmp::Obstacle>{}, Utils::Rnd::ExcludePack<Cmp::Wall, Cmp::Exit, Cmp::Player::Character, Cmp::Npc::NPC>{},
        new_rng_seed );

    const auto &wormhole_ms = m_sprite_factory.get_spritesheet_by_type( "sprite.graveyard.hazard.wormhole" );
    Cmp::Wormhole::MultiBlock wormhole_block( random_pos.position, wormhole_ms.get_px_size() );
    auto wormhole_bounds = Cmp::RectBounds::scaled( wormhole_block, 1.f );

    // Check collisions with walls, graves, shrines, and positions reserved from algorithmic changes
    using Utils::Collision::check_cmp;
    auto reserved_sm = m_reserved_sm.lock();
    bool is_valid = not check_cmp<Cmp::Wall>( reg(), wormhole_bounds ) && not check_cmp<Cmp::Grave::Segment>( reg(), wormhole_bounds ) &&
                    not check_cmp<Cmp::Altar::Segment>( reg(), wormhole_bounds ) &&
                    not check_cmp<Cmp::Crypt::BuildingSegment>( reg(), wormhole_bounds ) &&
                    not check_cmp<Cmp::Grave::ExitSegment>( reg(), wormhole_bounds ) &&
                    not check_cmp<Cmp::Hazard::FieldCell>( reg(), wormhole_bounds ) && ( not reserved_sm || reserved_sm->at( random_pos ).empty() );

    if ( is_valid )
    {
      if ( new_rng_seed != initial_rng_seed && initial_rng_seed > 0 )
      {
        SPDLOG_WARN( "Wormhole spawn: original seed {} was invalid, used seed {} instead (attempt {})", initial_rng_seed, new_rng_seed,
                     attempts + 1 );
      }
      return { random_entity, random_pos };
    }

    attempts++;
    if ( initial_rng_seed > 0 ) { new_rng_seed++; }
  }

  SPDLOG_ERROR( "Failed to find valid wormhole spawn location after {} attempts (original seed: {})", kMaxAttempts, initial_rng_seed );
  return { entt::null, Cmp::Position{ { 0.f, 0.f }, { 0.f, 0.f } } };
}

} // namespace Game::Sys