#include <Audio/SoundBank.hpp>
#include <Components/Npc/Shockwave.hpp>
#include <Components/Npc/ShockwaveTimer.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Persistent/NpcShockwaveMaxRadius.hpp>
#include <Components/Persistent/NpcShockwaveSpeed.hpp>
#include <Components/Persistent/PcDamageDelay.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/Character.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/Stats/ProjectileAction.hpp>
#include <Components/UUID.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Sprites/Shockwave.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Systems/Threats/WormholeSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <Systems/Threats/ShockwaveSystem.hpp>

namespace Game::Sys
{

ShockwaveSystem::ShockwaveSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory,
                                  Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank )
{
}

Sprites::Shockwave::CircleSegments ShockwaveSystem::split_segment_by_obstacle( const Sprites::CircleSegment &segment,
                                                                               const sf::FloatRect &obstacle_rect, sf::Vector2f shockwave_position,
                                                                               float radius, const int samples )
{
  Sprites::Shockwave::CircleSegments result;

  float angle_range = segment.get_end_angle() - segment.get_start_angle();
  const float inv_s = 1.0f / static_cast<float>( samples - 1 );

  int start_idx = -1;
  for ( int i = 0; i < samples; ++i )
  {
    float angle = segment.get_start_angle() + ( static_cast<float>( i ) * inv_s * angle_range );
    sf::Vector2f point = shockwave_position + sf::Vector2f( std::cos( angle ) * radius, std::sin( angle ) * radius );
    bool inside = obstacle_rect.contains( point );

    if ( !inside && start_idx == -1 ) { start_idx = i; }
    else if ( inside && start_idx != -1 )
    {
      float start_angle = segment.get_start_angle() + ( static_cast<float>( start_idx ) * inv_s * angle_range );
      float end_angle = segment.get_start_angle() + ( static_cast<float>( i - 1 ) * inv_s * angle_range );
      if ( end_angle > start_angle ) { result.emplace_back( start_angle, end_angle, true ); }
      start_idx = -1;
    }
  }

  if ( start_idx != -1 )
  {
    float start_angle = segment.get_start_angle() + ( static_cast<float>( start_idx ) * inv_s * angle_range );
    result.emplace_back( start_angle, segment.get_end_angle(), true );
  }

  return result;
}

bool ShockwaveSystem::intersects_with_visible_segments( const Cmp::Npc::Shockwave &shockwave, const sf::FloatRect &player_pos )
{
  sf::Vector2f position = shockwave.sprite.get_position();
  float radius = shockwave.sprite.get_radius();
  float outline_thickness = shockwave.sprite.get_outline_thickness();
  int points_per_segment = shockwave.sprite.get_points_per_segment();

  for ( const auto &segment : shockwave.sprite.get_visible_segments() )
  {
    float angle_range = segment.get_end_angle() - segment.get_start_angle();

    // Check both inner and outer radius points to account for thickness
    float inner_radius = radius - ( outline_thickness / 2.0f );
    float outer_radius = radius + ( outline_thickness / 2.0f );

    for ( int i = 0; i < points_per_segment; ++i )
    {
      float t = static_cast<float>( i ) / static_cast<float>( points_per_segment - 1 );
      float angle = segment.get_start_angle() + ( t * angle_range );

      auto cos_angle = std::cos( angle );
      auto sin_angle = std::sin( angle );
      sf::Vector2f inner_point = position + sf::Vector2f( cos_angle * inner_radius, sin_angle * inner_radius );
      sf::Vector2f outer_point = position + sf::Vector2f( cos_angle * outer_radius, sin_angle * outer_radius );

      if ( player_pos.contains( inner_point ) || player_pos.contains( outer_point ) )
      {
        // do shockwave/player knockback
        sf::Vector2f shockwave_direction( cos_angle, sin_angle );

        auto &player_pos_cmp = Utils::Player::get_position( reg() );
        auto new_position = Utils::snap_to_grid( player_pos_cmp.position + ( shockwave_direction.componentWiseMul( Constants::kGridSizePxF ) ) );
        SPDLOG_DEBUG( "Player position was {},{} - Knockback direction is {}, {} - New Position should be {},{}", player_pos_cmp.position.x,
                      player_pos_cmp.position.y, normalised_direction.x, normalised_direction.y, new_position.x, new_position.y );

        // make sure player isnt knocked into an obstacle
        bool is_valid = true;
        for ( auto [obstacle_entt, obstacle_cmp, obstacle_pos_cmp] : reg().view<Cmp::Obstacle, Cmp::Position>().each() )
        {
          if ( sf::FloatRect( new_position, Constants::kGridSizePxF ).findIntersection( obstacle_pos_cmp ) ) is_valid = false;
        }
        if ( is_valid ) { player_pos_cmp.position = new_position; }
        else { SPDLOG_DEBUG( "New Position was invalid so cancelled" ); }

        return true;
      }
    }
  }
  return false;
}

void ShockwaveSystem::remove_intersecting_segments( const sf::FloatRect &rect, Cmp::Npc::Shockwave &shockwave )
{
  Sprites::Shockwave::CircleSegments new_segments;

  for ( const auto &segment : shockwave.sprite.get_segments() )
  {
    if ( not segment.is_visible() ) continue;

    // Instead of deleting segments, rebuild the segment list without the intersecting segments
    Sprites::Shockwave::CircleSegments non_intersecting = Sys::ShockwaveSystem::split_segment_by_obstacle(
        segment, rect, shockwave.sprite.get_position(), shockwave.sprite.get_radius(), shockwave.sprite.get_points_per_segment() );
    new_segments.insert( new_segments.end(), non_intersecting.begin(), non_intersecting.end() );
  }

  shockwave.sprite.set_segments( std::move( new_segments ) );
}

void ShockwaveSystem::check_shockwave_player_collision()
{
  if ( not Utils::scene_setting<Cmp::SceneSettings::CollisionDetection>( reg() ).enabled ) return;

  // we need the projectile_action modifiers for this NPC type.
  auto &pc_damage_cooldown = Sys::PersistSystem::get<Cmp::Persist::PcDamageDelay>( reg() );
  auto player_view = reg().view<Cmp::Player::Character, Cmp::Position, Cmp::PlayerStats, Cmp::Player::Mortality>();

  for ( auto entt : reg().view<Cmp::Npc::Shockwave>() )
  {
    Cmp::Npc::Shockwave &shockwave = reg().get<Cmp::Npc::Shockwave>( entt );

    for ( auto [player_entity, player_cmp, player_pos, player_stats_cmp, player_mort_cmp] : player_view.each() )
    {
      // dont spam death events if the player is already dead
      if ( player_mort_cmp.state == Cmp::Player::Mortality::State::DEAD ) continue;
      if ( not player_cmp.skip_damage_cooldown_once &&
           player_cmp.m_damage_cooldown_timer.getElapsedTime().asSeconds() < pc_damage_cooldown.get_value() )
        continue;
      if ( intersects_with_visible_segments( shockwave, player_pos ) )
      {

        Utils::Player::apply_action_from_npc_store<Cmp::ProjectileAction>( reg(), "npc.priest" );
        player_cmp.skip_damage_cooldown_once = false;
        m_sound_bank.get_effect( "damage_player" ).play();
        player_cmp.m_damage_cooldown_timer.restart();
        SPDLOG_INFO( "Player (health:{}) INTERSECTS with Shockwave (position: {},{} - effective_radius: {})", player_stats_cmp.health(),
                     shockwave.sprite.get_position().x, shockwave.sprite.get_position().y, shockwave.sprite.get_radius() );

        // trigger death animation
        if ( player_stats_cmp.health() <= 0 )
        {
          if ( Utils::Player::player_has_extra_life( reg() ) )
          {
            Utils::Player::get_position( reg() ).position = Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( reg() );
            Factory::Player::remove_player_extra_life( reg() );
            m_sound_bank.get_effect( "player_respawn" ).play();
          }
          else { get_systems_event_queue().enqueue( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::SHOCKED, player_pos ) ); }
        }
      }
    }
  }
}

void ShockwaveSystem::update( sf::Time dt )
{
  // emit shockwaves from each NPC
  for ( auto [npc_entt, npc_cmp, anim_cmp, npc_pos_cmp, npc_uuid_cmp] : reg().view<Cmp::Npc::NPC, Cmp::AnimData, Cmp::Position, Cmp::UUID>().each() )
  {
    if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), npc_pos_cmp ) ) continue;
    if ( anim_cmp.m_sprite_type == "sprite.priest" )
    {
      // cooldown is handled in Factory function via Cmp::Npc::ShockwaveTimer per NPC
      if ( auto *shockwave_timer = reg().try_get<Cmp::Npc::ShockwaveTimer>( npc_entt ) ) *shockwave_timer += dt;
      auto created_shockwave = Factory::Npc::create_shockwave( reg(), npc_entt );
      if ( created_shockwave )
      {
        Factory::Particle::add_shockwave( reg(), "crypt.priest.particle.shockwave", npc_uuid_cmp, npc_pos_cmp.getCenter(), 50000 );
        m_sound_bank.get_effect( "emit_shockwave_low" ).play();
        m_sound_bank.get_effect( "emit_shockwave_high" ).play();
      }

      // update the shockwave position so that it follows NPC
      Factory::Particle::update_position( reg(), npc_uuid_cmp, npc_pos_cmp.getCenter() );
    }
  }

  // Invert the interval - larger values = faster updates, smaller values = slower updates
  auto speed_value = Sys::PersistSystem::get<Cmp::Persist::NpcShockwaveSpeed>( reg() ).get_value();
  auto max_radius = Sys::PersistSystem::get<Cmp::Persist::NpcShockwaveMaxRadius>( reg() );

  for ( auto entt : reg().view<Cmp::Npc::Shockwave>() )
  {
    auto &sw_cmp = reg().get<Cmp::Npc::Shockwave>( entt );
    float current_radius = sw_cmp.sprite.get_radius();

    // Exponential scaling - shockwave accelerates as it grows
    // This creates a natural acceleration that maintains the visual impression of constant speed as the circumference grows.
    float normalized_radius = current_radius / max_radius.get_value();
    float speed_multiplier = 1.0f + ( normalized_radius * normalized_radius ); // Quadratic acceleration
    float new_radius = current_radius + ( speed_value * speed_multiplier * dt.asSeconds() );
    sw_cmp.sprite.set_radius( new_radius );

    check_shockwave_obstacle_collision( entt, sw_cmp );

    if ( new_radius > max_radius.get_value() ) { reg().destroy( entt ); }
  }
  shockwave_update_clock.restart();

  check_shockwave_player_collision();
}

void ShockwaveSystem::check_shockwave_obstacle_collision( [[maybe_unused]] entt::entity shockwave_entity, Cmp::Npc::Shockwave &shockwave )
{
  auto obstacle_view = reg().view<Cmp::Obstacle, Cmp::Position, Cmp::AnimData>();

  for ( auto [obstacle_entity, obstacle_cmp, obstacle_pos, sprite_anim] : obstacle_view.each() )
  {
    // Check if this is the specific obstacle type and index we care about
    if ( ( sprite_anim.m_sprite_type == "sprite.crypt.wall.int.main" && sprite_anim.getFrameIndexOffset() == 1 ) or
         ( sprite_anim.m_sprite_type == "sprite.crypt.wall.int.main" && sprite_anim.getFrameIndexOffset() == 0 ) )
    {
      sf::FloatRect obstacle_rect( obstacle_pos.position, obstacle_pos.size );

      SPDLOG_DEBUG( "Checking obstacle at ({}, {}) size ({}, {}) against shockwave at ({}, {}) radius {}", obstacle_rect.position.x,
                    obstacle_rect.position.y, obstacle_rect.size.x, obstacle_rect.size.y, shockwave.sprite.getPosition().x,
                    shockwave.sprite.getPosition().y, shockwave.sprite.getRadius() );

      // Calculate distance from circle center to rectangle
      sf::Vector2f circle_center = shockwave.sprite.get_position();
      float circle_radius = shockwave.sprite.get_radius();

      // Find closest point on rectangle to circle center
      sf::Vector2f closest_point;
      closest_point.x = std::max( obstacle_rect.position.x, std::min( circle_center.x, obstacle_rect.position.x + obstacle_rect.size.x ) );
      closest_point.y = std::max( obstacle_rect.position.y, std::min( circle_center.y, obstacle_rect.position.y + obstacle_rect.size.y ) );

      // Calculate distance from circle center to closest point
      sf::Vector2f diff = circle_center - closest_point;
      float distance = std::sqrt( ( diff.x * diff.x ) + ( diff.y * diff.y ) );

      // Check if circle intersects with rectangle (accounting for outline thickness)
      float effective_radius = circle_radius + ( shockwave.sprite.get_outline_thickness() / 2.0f );

      if ( distance <= effective_radius )
      {
        SPDLOG_DEBUG( "Shockwave INTERSECTS with obstacle (distance: {}, effective_radius: {})", distance, effective_radius );
        Sys::ShockwaveSystem::remove_intersecting_segments( obstacle_rect, shockwave );
      }
      else { SPDLOG_DEBUG( "Shockwave does NOT intersect with obstacle (distance: {}, effective_radius: {})", distance, effective_radius ); }
    }
  }
}

} // namespace Game::Sys