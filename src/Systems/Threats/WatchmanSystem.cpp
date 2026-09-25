#include <Audio/SoundBank.hpp>
#include <Components/Direction.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/Skeleton.hpp>
#include <Components/Npc/Watchman.hpp>
#include <Components/Npc/WatchmanSearchlight.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Persistent/NpcWatchmanConeHalfAngle.hpp>
#include <Components/Persistent/NpcWatchmanConeLength.hpp>
#include <Components/Persistent/NpcWatchmanGunFireRate.hpp>
#include <Components/Persistent/NpcWatchmanGunReloadRate.hpp>
#include <Components/Persistent/NpcWatchmanIdleDirectionChangeInterval.hpp>
#include <Components/Persistent/NpcWatchmanSpawnCooldown.hpp>
#include <Components/Persistent/NpcWatchmanSpawnInfamy.hpp>
#include <Components/Persistent/NpcWatchmanSpawnMax.hpp>
#include <Components/Persistent/NpcWatchmanSweepAmplitude.hpp>
#include <Components/Persistent/NpcWatchmanSweepSpeed.hpp>
#include <Components/Persistent/PcDamageDelay.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Position.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/Stats/ProjectileAction.hpp>
#include <Components/UUID.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/System/Angle.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Systems/Threats/WatchmanSystem.hpp>
#include <Utils/Cardinal.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>
#include <cmath>

#include <spdlog/spdlog.h>

namespace Game::Sys
{

WatchmanSystem::WatchmanSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank )
{
  SPDLOG_DEBUG( "WatchmanSystem initialized" );
  m_watchman_spawn_timer = sf::Time::Zero;
}

void WatchmanSystem::update( sf::Time dt )
{
  m_watchman_spawn_timer += dt;
  if ( m_watchman_spawn_timer.asSeconds() >= Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanSpawnCooldown>( reg() ).get_value() )
  {
    spawn_watchman();
    m_watchman_spawn_timer = sf::Time::Zero;
  }

  update_searchlights( dt );

  if ( Utils::Player::get_mortality( reg() ).state == Cmp::Player::Mortality::State::ALIVE ) { update_gunfire( dt ); }

  // update the gunfire position so that it follows NPC
  for ( auto [npc_entt, npc_cmp, pos_cmp, uuid_cmp] : reg().view<Cmp::Npc::Watchman, Cmp::Position, Cmp::UUID>().each() )
  {
    Factory::Particle::update_position( reg(), uuid_cmp, pos_cmp.getCenter() );
  }

  check_gunfire_player_collision();
  check_gunfire_npc_collision();
}

void WatchmanSystem::update_searchlights( sf::Time dt )
{
  const auto sweep_speed = Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanSweepSpeed>( reg() ).get_value();
  const auto sweep_amplitude = Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanSweepAmplitude>( reg() ).get_value();
  const auto cone_half_angle = Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanConeHalfAngle>( reg() ).get_value();
  const auto cone_length = Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanConeLength>( reg() ).get_value();
  const auto idle_direction_change_interval = Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanIdleDirectionChangeInterval>( reg() ).get_value();
  const sf::Vector2f player_center = Utils::Player::get_position( reg() ).getCenter();

  auto watchman_view = reg().view<Cmp::Npc::Watchman, Cmp::Position, Cmp::Direction, Cmp::Npc::WatchmanSearchlight>();
  for ( auto [npc_entt, npc_cmp, npc_pos_cmp, dir_cmp, searchlight_cmp] : watchman_view.each() )
  {
    const sf::Vector2f npc_center = npc_pos_cmp.getCenter();

    if ( auto facing_angle = Utils::Maths::angle( dir_cmp ); facing_angle.has_value() )
    {
      // actively moving (chasing) — face the direction of travel, and reset the idle patrol timer
      // so a fresh interval starts once it stops again
      searchlight_cmp.last_facing_angle = facing_angle->asRadians();
      searchlight_cmp.idle_direction_timer = sf::Time::Zero;
    }
    else if ( not searchlight_cmp.locked_on_player )
    {
      // standing sentry — rotate through cardinal directions on a timer so the sweep patrols the area
      searchlight_cmp.idle_direction_timer += dt;
      if ( not searchlight_cmp.idle_direction_initialized or searchlight_cmp.idle_direction_timer.asSeconds() >= idle_direction_change_interval )
      {
        if ( searchlight_cmp.idle_direction_initialized ) { ++searchlight_cmp.idle_direction; }
        searchlight_cmp.idle_direction_initialized = true;

        searchlight_cmp.last_facing_angle = Utils::Maths::angle( searchlight_cmp.idle_direction.vector() ).value_or( sf::Angle::Zero ).asRadians();
        searchlight_cmp.idle_direction_timer = sf::Time::Zero;
      }
    }
    // else: locked onto the player but not moving this tick (e.g. between pathfinding ticks) — hold last_facing_angle

    searchlight_cmp.sweep_phase = Utils::Maths::normalizeAngle( searchlight_cmp.sweep_phase + ( dt.asSeconds() * sweep_speed ) );
    const float swept_angle = searchlight_cmp.last_facing_angle + ( std::sin( searchlight_cmp.sweep_phase ) * sweep_amplitude );
    const sf::Vector2f swept_direction{ std::cos( swept_angle ), std::sin( swept_angle ) };

    const auto to_player = Utils::Maths::normalized( player_center - npc_center );
    const float dist_to_player = Utils::Maths::getEuclideanDistance( npc_center, player_center );

    if ( searchlight_cmp.locked_on_player )
    {
      // stay locked onto the player, tracking them directly, as long as they remain in cone range —
      // detection/pursuit works through obstacles and multiblocks (the Watchman "senses" them);
      // only actually firing requires a clear shot, gated separately in update_gunfire
      if ( to_player.has_value() and dist_to_player < cone_length ) { searchlight_cmp.cone_direction = *to_player; }
      else
      {
        searchlight_cmp.locked_on_player = false;
        searchlight_cmp.cone_direction = swept_direction;
      }
    }
    else
    {
      searchlight_cmp.cone_direction = swept_direction;
      // once the sweeping cone catches the player, lock the beam onto them and start pursuit —
      // no line-of-sight requirement here, obstacles don't block detection, only gunfire
      if ( to_player.has_value() and Utils::Maths::is_point_in_cone( npc_center, swept_direction, cone_half_angle, cone_length, player_center ) )
      {
        searchlight_cmp.locked_on_player = true;
        searchlight_cmp.cone_direction = *to_player;
      }
    }
  }
}

void WatchmanSystem::update_gunfire( sf::Time dt )
{
  const auto gun_fire_rate = Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanGunFireRate>( reg() ).get_value();
  const auto gun_reload_rate = Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanGunReloadRate>( reg() ).get_value();
  const auto cone_half_angle = Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanConeHalfAngle>( reg() ).get_value();
  const auto cone_length = Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanConeLength>( reg() ).get_value();
  const sf::Vector2f player_center = Utils::Player::get_position( reg() ).getCenter();

  for ( auto [npc_entt, npc_cmp, npc_pos_cmp, npc_uuid_cmp, searchlight_cmp] :
        reg().view<Cmp::Npc::Watchman, Cmp::Position, Cmp::UUID, Cmp::Npc::WatchmanSearchlight>().each() )
  {
    const bool player_in_cone = Utils::Maths::is_point_in_cone( npc_pos_cmp.getCenter(), searchlight_cmp.cone_direction, cone_half_angle, cone_length,
                                                                player_center );
    const bool has_clear_shot = player_in_cone and has_line_of_sight( npc_pos_cmp.getCenter(), player_center );

    // Detection/pursuit works through obstacles, but cocking/firing requires an actual clear shot —
    // leaving the cone, or the player ducking behind an obstacle/multiblock, pauses (does not reset)
    // fire-rate/reload progress, so briefly breaking contact doesn't force the Watchman to restart
    // its cock/fire cycle.
    if ( not has_clear_shot ) continue;

    if ( not searchlight_cmp.gun_cocked )
    {
      if ( not searchlight_cmp.gun_ever_fired )
      {
        // fresh target acquisition — cock immediately, no reload delay before the first shot
        cock_gun( npc_pos_cmp );
        searchlight_cmp.gun_cocked = true;
        searchlight_cmp.gunfire_timer = sf::Time::Zero;
      }
      else
      {
        // reloading after a previous shot
        searchlight_cmp.gunfire_timer += dt;
        if ( searchlight_cmp.gunfire_timer.asSeconds() >= gun_reload_rate )
        {
          cock_gun( npc_pos_cmp );
          searchlight_cmp.gun_cocked = true;
          searchlight_cmp.gunfire_timer = sf::Time::Zero;
        }
      }
      continue;
    }

    // cocked, counting down to fire
    searchlight_cmp.gunfire_timer += dt;
    if ( searchlight_cmp.gunfire_timer.asSeconds() >= gun_fire_rate )
    {
      fire_gun( npc_pos_cmp, npc_uuid_cmp );
      searchlight_cmp.gun_cocked = false;
      searchlight_cmp.gun_ever_fired = true;
      searchlight_cmp.gunfire_timer = sf::Time::Zero;
    }
  }
}

bool WatchmanSystem::has_line_of_sight( sf::Vector2f from, sf::Vector2f to )
{
  for ( auto [blocker_entt, blocker_pos_cmp] : reg().view<Cmp::Player::NoPath, Cmp::Position>().each() )
  {
    if ( Utils::Maths::segment_intersects_rect( from, to, blocker_pos_cmp ) ) return false;
  }
  return true;
}

void WatchmanSystem::spawn_watchman()
{
  size_t watchman_npc_count = 0;
  for ( auto [npc_entt, npc_cmp] : reg().view<Cmp::Npc::NPC>().each() )
  {
    if ( reg().any_of<Cmp::Npc::Watchman>( npc_entt ) ) { watchman_npc_count++; }
  }

  if ( watchman_npc_count < Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanSpawnMax>( reg() ).get_value() and
       Utils::Player::get_stats( reg() ).infamy() >= Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanSpawnInfamy>( reg() ).get_value() )
  {
    auto reserved_sm = m_reserved_sm.lock();
    auto [rnd_entt, rnd_pos_cmp] = Utils::Rnd::get_random_position( reg(), {}, Utils::Rnd::ExcludePack<Cmp::Player::Character, Cmp::Obstacle>{}, 0 );
    if ( reserved_sm && not reserved_sm->at( rnd_pos_cmp ).empty() ) return;
    Factory::Npc::create_npc( reg(), rnd_entt, "npc.nightwatchman", reserved_sm );
    SPDLOG_INFO( "Spawned Watchman at {},{}", rnd_pos_cmp.x(), rnd_pos_cmp.y() );
  }
}

void WatchmanSystem::fire_gun( Cmp::Position &npc_pos_cmp, Cmp::UUID &npc_uuid_cmp )
{
  if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), npc_pos_cmp ) ) return;
  Factory::Particle::add_watchman_gunfire_ps( m_reg, "watchman.gun.particle.shot", 5.0, 100.f, npc_uuid_cmp, npc_pos_cmp.getCenter(), 50000 );
  if ( m_sound_bank.get_effect( "shotgun_fire" ).getStatus() != sf::Sound::Status::Playing ) m_sound_bank.get_effect( "shotgun_fire" ).play();
}

void WatchmanSystem::cock_gun( Cmp::Position &npc_pos_cmp )
{
  if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), npc_pos_cmp ) ) return;
  if ( m_sound_bank.get_effect( "shotgun_rack" ).getStatus() != sf::Sound::Status::Playing ) m_sound_bank.get_effect( "shotgun_rack" ).play();
}

void WatchmanSystem::check_gunfire_player_collision()
{
  if ( not Utils::scene_setting<Cmp::SceneSettings::CollisionDetection>( reg() ).enabled ) return;

  auto &pc_damage_cooldown = Sys::PersistSystem::get<Cmp::Persist::PcDamageDelay>( reg() );

  auto player_view = reg().view<Cmp::Player::Character, Cmp::Position, Cmp::PlayerStats, Cmp::Player::Mortality>();
  for ( auto [player_entt, player_cmp, player_pos_cmp, player_stats_cmp, player_mort_cmp] : player_view.each() )
  {
    // don't spam death events if the player is already dead
    if ( player_mort_cmp.state == Cmp::Player::Mortality::State::DEAD ) continue;
    if ( not player_cmp.skip_damage_cooldown_once &&
         player_cmp.m_damage_cooldown_timer.getElapsedTime().asSeconds() < pc_damage_cooldown.get_value() )
      continue;

    bool hit = false;
    for ( auto &gunfire_sprite : Sys::ParticleSystem::find( reg(), "watchman.gun.particle.shot" ) )
    {
      if ( gunfire_sprite.get().check_particle_collision( player_pos_cmp ) ) hit = true;
    }
    if ( not hit ) continue;

    Utils::Player::apply_action_from_npc_store<Cmp::ProjectileAction>( reg(), "npc.nightwatchman" );
    player_cmp.skip_damage_cooldown_once = false;
    m_sound_bank.get_effect( "damage_player" ).play();
    player_cmp.m_damage_cooldown_timer.restart();
    SPDLOG_INFO( "Player (health:{}) hit by Watchman gunfire", player_stats_cmp.health() );

    if ( player_stats_cmp.health() <= 0 )
    {
      if ( Utils::Player::player_has_extra_life( reg() ) )
      {
        Utils::Player::get_position( reg() ).position = Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( reg() );
        Factory::Player::remove_player_extra_life( reg() );
        m_sound_bank.get_effect( "player_respawn" ).play();
      }
      else { get_systems_event_queue().enqueue( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::HAUNTED, player_pos_cmp ) ); }
    }
  }
}

void WatchmanSystem::check_gunfire_npc_collision()
{
  if ( not Utils::scene_setting<Cmp::SceneSettings::CollisionDetection>( reg() ).enabled ) return;

  for ( auto [npc_entt, npc_cmp, skeleton_cmp, npc_pos_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Npc::Skeleton, Cmp::Position>().each() )
  {
    bool hit = false;
    for ( auto &gunfire_sprite : Sys::ParticleSystem::find( reg(), "watchman.gun.particle.shot" ) )
    {
      if ( gunfire_sprite.get().check_particle_collision( npc_pos_cmp ) ) hit = true;
    }
    if ( not hit ) continue;

    SPDLOG_INFO( "Skeleton NPC entity {} killed by Watchman gunfire", static_cast<int>( npc_entt ) );
    Factory::Npc::create_npc_death_anim( reg(), npc_pos_cmp, "sprite.death.anim.bloodsplat" );
    Factory::Npc::destroy_npc( reg(), npc_entt );
  }
}

} // namespace Game::Sys
