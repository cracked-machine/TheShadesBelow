
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
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>

#include <SFML/System/Vector2.hpp>

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

std::pair<entt::entity, Cmp::Position> WormholeSystem::find_spawn_location( unsigned long seed )
{
  constexpr int kMaxAttempts = 1000;
  int attempts = 0;
  unsigned long current_seed = seed;

  while ( attempts < kMaxAttempts )
  {
    auto [random_entity, random_pos] = Utils::Rnd::get_random_position(
        reg(), Utils::Rnd::IncludePack<Cmp::Obstacle>{}, Utils::Rnd::ExcludePack<Cmp::Wall, Cmp::Exit, Cmp::Player::Character, Cmp::Npc::NPC>{},
        current_seed );

    const auto &wormhole_ms = m_sprite_factory.get_spritesheet_by_type( "sprite.graveyard.hazard.wormhole" );
    Cmp::Wormhole::MultiBlock wormhole_block( random_pos.position, wormhole_ms.get_px_size() );

    // Check collisions with walls, graves, shrines
    auto is_valid = [&]() -> bool
    {
      // return false for wall collisions
      for ( auto [entity, wall_cmp, wall_pos_cmp] : reg().view<Cmp::Wall, Cmp::Position>().each() )
      {
        if ( wall_pos_cmp.findIntersection( wormhole_block ) ) return false;
      }

      // Return false for grave collisions
      for ( auto [entity, grave_cmp, grave_pos_cmp] : reg().view<Cmp::Grave::Segment, Cmp::Position>().each() )
      {
        if ( grave_pos_cmp.findIntersection( wormhole_block ) ) return false;
      }

      // Return false for altar collisions
      for ( auto [entity, altar_cmp, altar_pos_cmp] : reg().view<Cmp::Altar::Segment, Cmp::Position>().each() )
      {
        if ( altar_pos_cmp.findIntersection( wormhole_block ) ) return false;
      }

      // Return false for crypt collisions
      for ( auto [entity, crypt_cmp, crypt_pos_cmp] : reg().view<Cmp::Crypt::BuildingSegment, Cmp::Position>().each() )
      {
        if ( crypt_pos_cmp.findIntersection( wormhole_block ) ) return false;
      }

      // Return false for graveyard exit collisions
      for ( auto [entity, xit_cmp, exit_pos_cmp] : reg().view<Cmp::Grave::ExitSegment, Cmp::Position>().each() )
      {
        if ( exit_pos_cmp.findIntersection( wormhole_block ) ) return false;
      }

      // Return false for hazard collisions
      for ( auto [entity, hazard_cmp, hazard_pos_cmp] : reg().view<Cmp::Hazard::FieldCell, Cmp::Position>().each() )
      {
        if ( hazard_pos_cmp.findIntersection( wormhole_block ) ) return false;
      }

      // Return false for positions reserved from algorithmic changes. Checked against the
      // candidate's own cell only (matching the pre-refactor Cmp::ReservedPosition exclude
      // semantics) rather than the whole wormhole footprint - query_rect over the full 3x3
      // block requires all 9 cells to be simultaneously unreserved, which is far stricter than
      // before and can make a valid spawn very hard to find on a densely decorated map.
      if ( auto reserved_sm = m_reserved_sm.lock(); reserved_sm && not reserved_sm->at( random_pos ).empty() )
      {
        return false;
      }

      return true;
    };

    if ( is_valid() )
    {
      if ( current_seed != seed && seed > 0 )
      {
        SPDLOG_WARN( "Wormhole spawn: original seed {} was invalid, used seed {} instead (attempt {})", seed, current_seed, attempts + 1 );
      }
      return { random_entity, random_pos };
    }

    attempts++;
    // Increment seed for next attempt (works for both seeded and non-seeded cases)
    if ( seed > 0 ) { current_seed++; }
  }

  SPDLOG_ERROR( "Failed to find valid wormhole spawn location after {} attempts (original seed: {})", kMaxAttempts, seed );
  return { entt::null, Cmp::Position{ { 0.f, 0.f }, { 0.f, 0.f } } };
}

void WormholeSystem::spawn_wormhole( SpawnPhase phase )
{
  // 1. pick a random position component in the maze, exclude walls, doors, exits, and playable
  // characters
  // 2. get the entity at that position
  unsigned long seed = 0;
  if ( phase == SpawnPhase::InitialSpawn ) seed = Sys::PersistSystem::get<Cmp::Persist::WormholeSeed>( reg() ).get_value();

  auto [random_entity, random_pos] = find_spawn_location( seed );
  if ( random_entity == entt::null )
  {
    SPDLOG_ERROR( "Failed to find valid wormhole spawn position." );
    return;
  }

  // 3. Create the sprite
  const auto &wormhole_ss = m_sprite_factory.get_spritesheet_by_type( "sprite.graveyard.hazard.wormhole" );
  Cmp::Wormhole::MultiBlock wormhole_block( random_pos.position, wormhole_ss.get_px_size() );

  auto navmesh = m_npc_navmesh.lock();
  auto reserved_sm = m_reserved_sm.lock();
  for ( auto [entity, obstacle_pos] : reg().view<Cmp::Position>().each() )
  {
    if ( obstacle_pos.findIntersection( wormhole_block ) )
    {
      bool was_obstacle = reg().all_of<Cmp::Npc::NoPathFinding>( entity );
      Factory::Obstacle::remove_obstacle( reg(), entity );
      Factory::Loot::destroy_loot_container( reg(), entity, reserved_sm );
      Factory::Npc::destroy_npc_container( reg(), entity, reserved_sm );
      if ( was_obstacle && navmesh ) navmesh->insert( entity, obstacle_pos );

      SPDLOG_DEBUG( "Wormhole spawn: Destroying item at ({}, {})", obstacle_pos.position.x, obstacle_pos.position.y );
    }
  }

  auto uuid_cmp = Cmp::UUID::generate();

  // 4. add the wormhole component to the entity
  // get the erntity that owns the center grid position of the 3x3 area
  sf::Vector2f center_pos = random_pos.position + Constants::kGridSizePxF;
  auto center_entity = reg().create();
  reg().emplace<Cmp::Position>( center_entity, center_pos, Constants::kGridSizePxF );
  reg().emplace<Cmp::Wormhole::Singularity>( center_entity );

  // getReg().emplace_or_replace<Cmp::Wormhole::Singularity>( random_entity );
  reg().emplace_or_replace<Cmp::Wormhole::MultiBlock>( random_entity, random_pos.position, wormhole_ss.get_px_size() );
  // clang-format off
  reg().emplace_or_replace<Cmp::AnimData>( random_entity, Cmp::AnimData::Config{  
        .sprite_type = "sprite.graveyard.hazard.wormhole",
        .enabled = true
  });
  // clang-format on
  reg().emplace_or_replace<Cmp::UUID>( random_entity, uuid_cmp.data );
  reg().emplace_or_replace<Cmp::ZOrderValue>( random_entity, random_pos.position.y - 16 );

  Factory::Particle::add_wormhole_ps( reg(), "graveyard.wormhole.particles", 1.f, 25.f, uuid_cmp, sf::Vector2f( center_pos.x + 8, center_pos.y + 8 ),
                                      5000.f );

  SPDLOG_INFO( "Wormhole spawned at position ({}, {}) with zorder: {}", random_pos.position.x, random_pos.position.y,
               random_pos.position.y - random_pos.size.y );
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
    // TODO: pointless check? Never happens (according to log)
    if ( !jump_pos_cmp )
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

      // Get unique random position for this actor entity
      auto [new_spawn_entity, new_spawn_pos_cmp] = Utils::Rnd::get_random_position(
          reg(), Utils::Rnd::IncludePack<Cmp::Obstacle>{}, Utils::Rnd::ExcludePack<Cmp::Wall, Cmp::Exit, Cmp::Player::Character, Cmp::Npc::NPC>{},
          0 );

      Factory::Obstacle::remove_obstacle( reg(), new_spawn_entity, Factory::Obstacle::DeleteExtras::Yes, m_reserved_sm.lock() );
      if ( auto teleport_navmesh = m_npc_navmesh.lock() ) teleport_navmesh->insert( new_spawn_entity, new_spawn_pos_cmp );
      // clang-format off
      reg().emplace_or_replace<Cmp::AnimData>( new_spawn_entity, Cmp::AnimData::Config{
            .sprite_type = "sprite.graveyard.detonated",
            .enabled = true
      });
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
    for ( auto [ps_entt, ps_cmp, ps_uuid_cmp] : reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
    {
      if ( ps_cmp.sprite->get_tag() == "graveyard.wormhole.particles" ) reg().destroy( ps_entt );
    }
    spawn_wormhole( WormholeSystem::SpawnPhase::Respawn );
  }
}

void WormholeSystem::despawn_wormhole()
{
  // remove the wormhole entity
  auto wormhole_view = reg().view<Cmp::Wormhole::Singularity>();
  for ( auto [entity, _] : wormhole_view.each() )
  {
    reg().remove<Cmp::Wormhole::Singularity>( entity );
    SPDLOG_DEBUG( "Wormhole despawned (entity {})", static_cast<uint32_t>( entity ) );
  }

  auto wormhole_mb_view = reg().view<Cmp::Wormhole::MultiBlock>();
  for ( auto [entity, _] : wormhole_mb_view.each() )
  {
    reg().remove<Cmp::Wormhole::MultiBlock>( entity );
    reg().remove<Cmp::AnimData>( entity );
    SPDLOG_DEBUG( "MultiBlock despawned (entity {})", static_cast<uint32_t>( entity ) );
  }
}

} // namespace Game::Sys