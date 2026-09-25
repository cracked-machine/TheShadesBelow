#include <Audio/SoundBank.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/AnimData.hpp>
#include <Components/Crypt/BuildingSegment.hpp>
#include <Components/Crypt/ObjectiveSegment.hpp>
#include <Components/DeathPosition.hpp>
#include <Components/Direction.hpp>
#include <Components/FootStepTimer.hpp>
#include <Components/Grave/ExitSegment.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Grave/Segment.hpp>
#include <Components/LerpPosition.hpp>
#include <Components/Npc/Container.hpp>
#include <Components/Npc/Friendly.hpp>
#include <Components/Npc/Ghost.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/Shockwave.hpp>
#include <Components/Npc/WatchmanSearchlight.hpp>
#include <Components/Npc/Wisp.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Persistent/NpcActivateScale.hpp>
#include <Components/Persistent/NpcShockwaveMaxRadius.hpp>
#include <Components/Persistent/NpcShockwaveSpeed.hpp>
#include <Components/Persistent/PcDamageDelay.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Ruin/BuildingSegment.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/SpawnArea.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/CollisionAction.hpp>

#include <Components/UUID.hpp>
#include <Components/Wall.hpp>
#include <Components/Wormhole/Jump.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/SpriteFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Threats/NpcSystem.hpp>
#include <Systems/Threats/ShockwaveSystem.hpp>
#include <Utils/Cardinal.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Npc.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Audio/Sound.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Time.hpp>
#include <cmath>
#include <spdlog/spdlog.h>

namespace Game::Sys
{

NpcSystem::NpcSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank )
{
  SPDLOG_DEBUG( "NpcSystem initialized" );
}

void NpcSystem::update( sf::Time dt )
{

  // remove any finished explosions
  for ( auto [death_entt, death_cmp, anim_cmp] : reg().view<Cmp::DeathPosition, Cmp::AnimData>().each() )
  {
    if ( not anim_cmp.m_enabled ) { Factory::Npc::remove_npc_death_anim( reg(), death_entt ); }
  }

  static constexpr float kContainerInterval = 0.20f;
  m_container_timer += dt;
  if ( m_container_timer.asSeconds() >= kContainerInterval )
  {
    check_npc_container_collision();
    m_container_timer = sf::Time::Zero;
  }

  update_pathfinding( dt );

  static constexpr float kAnimInterval = 0.05f;
  m_anim_timer += dt;
  if ( m_anim_timer.asSeconds() >= kAnimInterval )
  {
    update_animation();
    m_anim_timer = sf::Time::Zero;
  }
  update_sfx();
  update_movement( dt );

  if ( Utils::scene_setting<Cmp::SceneSettings::CollisionDetection>( reg() ).enabled )
  {
    check_once_collision();
    check_timed_collision( dt );
  }
}

void NpcSystem::check_npc_container_collision()
{

  auto player_pos = Utils::Player::get_position( reg() );
  auto npccontainer_collision_view = reg().view<Cmp::Npc::Container, Cmp::Position>();
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [npccontainer_entt, npccontainer_cmp, npccontainer_pos_cmp] : npccontainer_collision_view.each() )
  {
    if ( !Utils::is_visible_in_view( view_bounds, npccontainer_pos_cmp ) ) continue;

    auto &npc_activate_scale = Sys::PersistSystem::get<Cmp::Persist::NpcActivateScale>( reg() );
    // we just create a temporary RectBounds here instead of a component because we only need it
    // for this one comparison and it already contains the needed scaling logic
    auto npc_activate_bounds = Cmp::RectBounds::scaled( npccontainer_pos_cmp.position, Constants::kGridSizePxF, npc_activate_scale.get_value() );

    if ( player_pos.findIntersection( npc_activate_bounds.getBounds() ) )
    {
      Factory::Npc::create_npc( reg(), npccontainer_entt, "npc.skeleton", m_reserved_sm.lock() );
    }
  }
}

void NpcSystem::update_animation()
{
  for ( auto [npc_entt, npc_cmp, npc_dir_cmp, anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Direction, Cmp::AnimData>().each() )
  {
    // Watchmen face wherever their searchlight is currently pointing — sweeping, patrolling a
    // cardinal direction, or locked onto the player — even while standing still, rather than
    // freezing on whatever direction they last walked.
    if ( auto *searchlight_cmp = reg().try_get<Cmp::Npc::WatchmanSearchlight>( npc_entt ) )
    {
      anim_cmp.m_enabled = npc_dir_cmp != sf::Vector2f( 0.0f, 0.0f );
      const sf::Vector2f facing = searchlight_cmp->cone_direction;
      if ( std::abs( facing.x ) > std::abs( facing.y ) )
      {
        anim_cmp.m_sprite_type = facing.x > 0 ? "sprite.nightwatchman.walk.east" : "sprite.nightwatchman.walk.west";
      }
      else { anim_cmp.m_sprite_type = facing.y < 0 ? "sprite.nightwatchman.walk.north" : "sprite.nightwatchman.walk.south"; }
      continue;
    }

    if ( npc_dir_cmp == sf::Vector2f( 0.0f, 0.0f ) )
    {
      anim_cmp.m_enabled = false;
      continue;
    }

    anim_cmp.m_enabled = true;
    if ( anim_cmp.m_sprite_type.contains( "sprite.skeleton" ) )
    {

      if ( npc_dir_cmp.x > 0 ) { anim_cmp.m_sprite_type = "sprite.skeleton.walk.east"; }
      else if ( npc_dir_cmp.x < 0 ) { anim_cmp.m_sprite_type = "sprite.skeleton.walk.west"; }
      else if ( npc_dir_cmp.y < 0 ) { anim_cmp.m_sprite_type = "sprite.skeleton.walk.north"; }
      else if ( npc_dir_cmp.y > 0 ) { anim_cmp.m_sprite_type = "sprite.skeleton.walk.south"; }
    }
    else if ( anim_cmp.m_sprite_type.contains( "sprite.ghost" ) )
    {
      if ( npc_dir_cmp.x > 0 ) { anim_cmp.m_sprite_type = "sprite.ghost.walk.east"; }
      else if ( npc_dir_cmp.x < 0 ) { anim_cmp.m_sprite_type = "sprite.ghost.walk.west"; }
      else if ( npc_dir_cmp.y < 0 ) { anim_cmp.m_sprite_type = "sprite.ghost.walk.north"; }
      else if ( npc_dir_cmp.y > 0 ) { anim_cmp.m_sprite_type = "sprite.ghost.walk.south"; }
    }
    else if ( anim_cmp.m_sprite_type.contains( "sprite.wisp" ) )
    {
      if ( npc_dir_cmp.x > 0 ) { anim_cmp.m_sprite_type = "sprite.wisp.east"; }
      else if ( npc_dir_cmp.x < 0 ) { anim_cmp.m_sprite_type = "sprite.wisp.west"; }
    }
  }
}

void NpcSystem::update_sfx()
{
  bool any_skeleton_moving = false;
  bool any_spider_moving = false;

  for ( auto [npc_entt, npc_cmp, npc_dir_cmp, anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Direction, Cmp::AnimData>().each() )
  {
    if ( anim_cmp.m_sprite_type.contains( "sprite.skeleton" ) )
    {
      if ( npc_dir_cmp != sf::Vector2f{ 0.0f, 0.0f } )
      {
        any_skeleton_moving = true;
        break;
      }
    }
    else if ( anim_cmp.m_sprite_type.contains( "sprite.spider" ) )
    {
      if ( npc_dir_cmp != sf::Vector2f{ 0.0f, 0.0f } )
      {
        any_spider_moving = true;
        break;
      }
    }
  }

  auto &skeleton_sfx = m_sound_bank.get_effect( "skeleton_moving" );
  if ( any_skeleton_moving )
  {
    if ( skeleton_sfx.getStatus() != sf::Sound::Status::Playing ) { skeleton_sfx.play(); }
  }
  else { skeleton_sfx.stop(); }

  auto &spider_sfx = m_sound_bank.get_effect( "spider_moving" );
  if ( any_spider_moving )
  {
    if ( spider_sfx.getStatus() != sf::Sound::Status::Playing ) { spider_sfx.play(); }
  }
  else { spider_sfx.stop(); }
}

void NpcSystem::update_pathfinding( sf::Time dt )
{
  static constexpr float kPathfindingInterval = 0.10f;
  m_pathfinding_timer += dt;
  if ( m_pathfinding_timer.asSeconds() >= kPathfindingInterval )
  {
    // NPCs — target is always the player; compute spawn check once for all
    const Cmp::Position player_pos = Utils::Player::get_position( reg() );
    const bool player_in_spawn = Utils::Player::is_in_spawn( reg(), player_pos );
    const bool player_is_illuminated = Utils::Player::is_illuminated( reg() );
    for ( auto [npc_entt, npc_cmp] : reg().view<Cmp::Npc::NPC>().each() )
    {
      if ( reg().any_of<Cmp::Npc::Wisp>( npc_entt ) ) continue;

      // Watchmen stand sentry, sweeping their searchlight, until they actually catch the player
      // in the beam — only then do they give chase.
      auto *searchlight_cmp = reg().try_get<Cmp::Npc::WatchmanSearchlight>( npc_entt );
      if ( searchlight_cmp and not searchlight_cmp->locked_on_player )
      {
        reg().emplace_or_replace<Cmp::Direction>( npc_entt, Cmp::Direction( { 0.0f, 0.0f } ) );
        continue;
      }

      // Skip NPCs already stopped at the spawn boundary — A* result won't change, since spawn areas
      // are static geometry. Watchmen are exempt: they may still be sitting at Direction {0,0} from
      // sentry mode the instant they lock onto the player, and must always re-path rather than being
      // mistaken for an NPC that already settled on "no path needed".
      // Illumination is deliberately NOT included here: the light boundary can move (the player's own
      // inventory candle tracks their position, wisps move) so a halted NPC must keep re-checking the
      // boundary every tick rather than being trusted to stay correctly parked.
      if ( player_in_spawn and not searchlight_cmp )
      {
        auto *npc_dir = reg().try_get<Cmp::Direction>( npc_entt );
        auto *npc_lerp = reg().try_get<Cmp::LerpPosition>( npc_entt );
        if ( npc_dir && npc_dir->x == 0.0f && npc_dir->y == 0.0f && !npc_lerp ) continue;
      }
      auto navmesh = navmesh_for( npc_entt );
      if ( not navmesh ) continue;
      update_pathfinding_for( *navmesh, player_pos, npc_entt, player_in_spawn, player_is_illuminated );
    }

    m_pathfinding_timer = sf::Time::Zero;
  }
}
void NpcSystem::update_pathfinding_for( PathFinding::SpatialHashGrid &navmesh, const Cmp::Position &target_pos, entt::entity npc_entity,
                                        bool target_in_spawn, bool target_illuminated )
{
  auto result = Utils::Npc::pathfind_toward( reg(), navmesh, target_pos, npc_entity, target_in_spawn, target_illuminated );
  if ( result == Utils::Npc::PathfindResult::NoPath ) { reg().emplace_or_replace<Cmp::Direction>( npc_entity, Cmp::Direction( { 0.0, 0.0 } ) ); }
}

void NpcSystem::update_movement( sf::Time dt )
{

  for ( auto [npc_entt, npc_cmp, npc_pos_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position>().each() )
  {
    auto navmesh = navmesh_for( npc_entt );
    if ( not navmesh ) continue;
    update_movement_for( *navmesh, npc_entt, dt );
  }
}

PathFinding::SpatialHashGridSharedPtr NpcSystem::navmesh_for( entt::entity npc_entity )
{
  if ( reg().any_of<Cmp::Npc::Wisp>( npc_entity ) ) { return m_open_navmesh.lock(); }
  if ( reg().any_of<Cmp::Npc::Ghost>( npc_entity ) )
  {
    // ghosts pass through plants; fall back to the regular navmesh in scenes without one
    if ( auto ghost_navmesh = m_ghost_navmesh.lock() ) { return ghost_navmesh; }
  }
  return m_npc_navmesh.lock();
}

void NpcSystem::update_movement_for( PathFinding::SpatialHashGrid &navmesh, entt::entity npc_entity, sf::Time dt )
{

  // if there is an obstacle at this entity move onto the next entity
  auto *obst_cmp = reg().try_get<Cmp::Obstacle>( npc_entity );
  if ( obst_cmp ) return;

  // If this is the first update, store the start position
  auto *lerp_pos_cmp = reg().try_get<Cmp::LerpPosition>( npc_entity );
  auto *pos_cmp = reg().try_get<Cmp::Position>( npc_entity );
  if ( not pos_cmp or not lerp_pos_cmp ) return;
  if ( lerp_pos_cmp->m_lerp_factor == 0.0f )
  {
    // Allow NPCs to escape wormholes if they're mid-lerp.
    if ( reg().try_get<Cmp::Wormhole::Jump>( npc_entity ) ) return;

    lerp_pos_cmp->m_start = pos_cmp->position;
  }

  lerp_pos_cmp->m_lerp_factor += lerp_pos_cmp->m_lerp_speed * dt.asSeconds();

  // lerp has completed
  if ( lerp_pos_cmp->m_lerp_factor >= 1.0f )
  {
    auto old_position = *pos_cmp;
    pos_cmp->position = lerp_pos_cmp->m_target;
    reg().remove<Cmp::LerpPosition>( npc_entity );
    navmesh.update( npc_entity, old_position, *pos_cmp );
  }
  else
  {
    // Lerp from start to target directly
    // Simple manual lerp - 33 lines of assembly vs 134 for std::lerp vs 54 for std::fma - https://godbolt.org/z/YdeKco5d6
    const float t = lerp_pos_cmp->m_lerp_factor;
    const float one_minus_t = 1.0f - t;
    pos_cmp->position.x = one_minus_t * lerp_pos_cmp->m_start.x + t * lerp_pos_cmp->m_target.x;
    pos_cmp->position.y = one_minus_t * lerp_pos_cmp->m_start.y + t * lerp_pos_cmp->m_target.y;
  }

  // add additional zorder for Wisp NPCs (read from JSON)
  float zorder_augment = 0.f;
  if ( Utils::Npc::get_sprite_type( reg(), npc_entity ).contains( "wisp" ) )
  {
    const auto &spritesheet = m_sprite_factory.get_spritesheet_by_type( "sprite.wisp.east" );
    zorder_augment = spritesheet.get_zorder( 0 );
  }
  reg().patch<Cmp::ZOrderValue>( npc_entity, [&]( auto &zorder_cmp ) { zorder_cmp.setZOrder( pos_cmp->position.y + zorder_augment ); } );
}

void NpcSystem::check_once_collision()
{
  auto player_collision_view = reg().view<Cmp::Player::Character>();
  auto npc_collision_view = reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::Direction>( entt::exclude<Cmp::Npc::Friendly> );

  auto &player_dmg_cooldown = Sys::PersistSystem::get<Cmp::Persist::PcDamageDelay>( reg() );
  auto &player_pos = Utils::Player::get_position( reg() );
  auto &player_mort = Utils::Player::get_mortality( reg() );
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [player_entity, player_cmp] : player_collision_view.each() )
  {
    if ( player_mort.state != Cmp::Player::Mortality::State::ALIVE ) return;
    for ( auto [npc_entity, npc_cmp, npc_pos_cmp, npc_dir_cmp] : npc_collision_view.each() )
    {
      if ( not Utils::is_visible_in_view( view_bounds, npc_pos_cmp ) ) continue;

      // relaxed bounds to allow player to sneak past during lerp transition
      auto npc_pos_cmp_bounds_current = Cmp::RectBounds::scaled( npc_pos_cmp.position, npc_pos_cmp.size, 0.1f );
      const bool touching_player = player_pos.findIntersection( npc_pos_cmp_bounds_current.getBounds() ).has_value();

      // a Watchman that's touched the player obviously knows exactly where they are — snap its
      // searchlight (and, since sprite facing already follows cone_direction, its sprite too) to
      // face them directly. This must happen on every physical touch, independent of the
      // player's damage-cooldown/NPC collision-action-interval gates below — those only throttle
      // *damage*, and gating detection on them meant a touch during someone else's cooldown
      // would silently fail to turn the Watchman.
      if ( touching_player )
      {
        if ( auto *searchlight_cmp = reg().try_get<Cmp::Npc::WatchmanSearchlight>( npc_entity ) )
        {
          if ( auto to_player = Utils::Maths::normalized( player_pos.getCenter() - npc_pos_cmp.getCenter() ); to_player.has_value() )
          {
            searchlight_cmp->locked_on_player = true;
            searchlight_cmp->cone_direction = *to_player;
          }
        }
      }

      if ( not player_cmp.skip_damage_cooldown_once &&
           player_cmp.m_damage_cooldown_timer.getElapsedTime().asSeconds() < player_dmg_cooldown.get_value() )
        continue;

      auto npc_collision_action = npc_cmp.at<Cmp::CollisionAction>();
      auto &[action, timer] = npc_collision_action;
      if ( action.interval() > 0.f ) continue;

      if ( not touching_player ) continue;

      Utils::Player::get_stats( reg() ).apply( action );
      player_cmp.skip_damage_cooldown_once = false;

      m_sound_bank.get_effect( "damage_player" ).play();

      if ( check_player_death( player_mort ) ) return;

      player_cmp.m_damage_cooldown_timer.restart();

      find_pushback_position( npc_dir_cmp );
    }
  }
}

void NpcSystem::check_timed_collision( sf::Time dt )
{
  auto npc_collision_view = reg().view<Cmp::Npc::NPC, Cmp::Position>( entt::exclude<Cmp::Npc::Friendly> );

  auto &player_pos = Utils::Player::get_position( reg() );
  auto &player_mort = Utils::Player::get_mortality( reg() );
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  if ( player_mort.state != Cmp::Player::Mortality::State::ALIVE ) return;
  for ( auto [npc_entity, npc_cmp, npc_pos_cmp] : npc_collision_view.each() )
  {
    if ( not Utils::is_visible_in_view( view_bounds, npc_pos_cmp ) ) continue;
    auto &npc_collision_action = npc_cmp.at<Cmp::CollisionAction>();

    auto &[npc_action, npc_action_timer] = npc_collision_action;
    if ( npc_action.interval() == 0.f ) continue;
    npc_action_timer += dt;
    if ( npc_action_timer.asSeconds() < npc_action.interval() ) continue;

    if ( not player_pos.findIntersection( npc_pos_cmp ) ) continue;

    Utils::Player::get_stats( reg() ).apply( npc_action );
    if ( m_sound_bank.get_effect( "damage_player" ).getStatus() != sf::Sound::Status::Playing ) { m_sound_bank.get_effect( "damage_player" ).play(); }

    npc_action_timer = sf::Time::Zero;

    if ( check_player_death( player_mort ) ) return;
  }
}

bool NpcSystem::check_player_death( Cmp::Player::Mortality &player_mort )
{
  if ( Utils::Player::get_stats( reg() ).health() > 0 ) return false;

  player_mort.state = Cmp::Player::Mortality::State::HAUNTED;
  get_systems_event_queue().enqueue( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::HAUNTED, Utils::Player::get_position( reg() ) ) );
  return true;
}

void NpcSystem::find_pushback_position( const Cmp::Direction &npc_direction )
{
  auto &player_pos = Utils::Player::get_position( reg() );

  // returns the name of the component blocking `rect`, or an empty string if it's clear
  auto blocking_component_name = [&]( const Cmp::RectBounds &rect ) -> std::string
  {
    if ( Utils::Collision::any_intersects<Cmp::Obstacle>( reg(), rect ) ) return "Obstacle";
    if ( Utils::Collision::any_intersects<Cmp::PlantSegment>( reg(), rect ) ) return "PlantSegment";
    if ( Utils::Collision::any_intersects<Cmp::Wall>( reg(), rect ) ) return "Wall";
    if ( Utils::Collision::any_intersects<Cmp::Altar::Segment>( reg(), rect ) ) return "Segment";
    if ( Utils::Collision::any_intersects<Cmp::Grave::Segment>( reg(), rect ) ) return "Segment";
    if ( Utils::Collision::any_intersects<Cmp::Grave::ExitSegment>( reg(), rect ) ) return "ExitSegment";
    if ( Utils::Collision::any_intersects<Cmp::Crypt::BuildingSegment>( reg(), rect ) ) return "BuildingSegment";
    if ( Utils::Collision::any_intersects<Cmp::Ruin::BuildingSegment>( reg(), rect ) ) return "BuildingSegment";
    if ( Utils::Collision::any_intersects<Cmp::Crypt::ObjectiveSegment>( reg(), rect ) ) return "ObjectiveSegment";
    return {};
  };

  // Try the NPC's own approach direction first; if that cell is blocked, fall back through
  // the remaining cardinal directions instead of giving up on knockback entirely.
  const sf::Vector2f primary_direction = npc_direction;

  std::vector<sf::Vector2f> candidate_directions{ primary_direction };
  for ( auto c : Utils::Cardinal() )
  {
    if ( c.vector() != primary_direction ) candidate_directions.push_back( c.vector() );
  }

  for ( const auto &direction : candidate_directions )
  {
    auto new_position = Utils::snap_to_grid( player_pos.position + direction.componentWiseMul( Constants::kGridSizePxF ) );
    auto new_pos_rect = Cmp::RectBounds::scaled( new_position, Constants::kGridSizePxF, 1.f );

    auto blocker = blocking_component_name( new_pos_rect );
    if ( not blocker.empty() )
    {
      SPDLOG_INFO( "Knockback direction {},{} blocked by {} at {},{}", direction.x, direction.y, blocker, new_position.x, new_position.y );
      continue;
    }

    SPDLOG_INFO( "Knockback succeeded: player moved to {},{}", new_position.x, new_position.y );
    player_pos.position = new_position;
    return;
  }

  SPDLOG_INFO( "Knockback blocked in all directions" );
}

} // namespace Game::Sys