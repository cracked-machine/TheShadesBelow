#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteAlpha.hpp>
#include <Components/AbsoluteOffset.hpp>
#include <Components/AbsoluteRenderOffset.hpp>
#include <Components/AbsoluteRotation.hpp>
#include <Components/Altar/AltarActivationTimer.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/AnimData.hpp>
#include <Components/Crypt/Chest.hpp>
#include <Components/Crypt/RoomLavaPitCell.hpp>
#include <Components/Direction.hpp>
#include <Components/Exit.hpp>
#include <Components/FootStepTimer.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Hazard/CollisionResist.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/ScryingBall.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LastDirection.hpp>
#include <Components/LerpPosition.hpp>
#include <Components/Moveable.hpp>
#include <Components/NoMoveDest.hpp>
#include <Components/NoRender.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/Wisp.hpp>
#include <Components/Obstacle.hpp>
#include <Components/ObstacleCap.hpp>
#include <Components/Particle/BlockParticle.hpp>
#include <Components/Particle/SpriteBase.hpp>
#include <Components/Persistent/PcDamageDelay.hpp>
#include <Components/Persistent/PlayerAnimStrideLength.hpp>
#include <Components/Persistent/PlayerDiagonalLerpSpeedModifier.hpp>
#include <Components/Persistent/PlayerMovementSpeed.hpp>
#include <Components/Persistent/PlayerShortcutLerpSpeedModifier.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Persistent/PostPullMovementDelay.hpp>
#include <Components/Persistent/WeaponDegradePerHit.hpp>
#include <Components/Plant/BurningTimeAccumulator.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/DiggingTimer.hpp>
#include <Components/Player/Illuminated.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Player/MovementDelta.hpp>
#include <Components/Player/MovementSuppressTimer.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Player/PendingNoPath.hpp>
#include <Components/Player/PostDeathTimeout.hpp>
#include <Components/Player/TorchRadius.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Ruin/Cobweb.hpp>
#include <Components/Ruin/RuneMarking.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/SelectedPosition.hpp>
#include <Components/Spring/ActiveHealing.hpp>
#include <Components/Spring/HealingSpringMultiBlock.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/CollisionAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/PlayerStats.hpp>
#include <Components/Stats/ProjectileAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Components/UUID.hpp>
#include <Components/Wall.hpp>
#include <Components/Wormhole/Jump.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Factory/LootFactory.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PlantFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Factory/SpriteFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PlayerSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>
#include <string>
#include <typeindex>

namespace Game::Sys
{

PlayerSystem::PlayerSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank,
                            entt::dispatcher &scenemanager_event_dispatcher )
    : BaseSystem( reg, window, sprite_factory, sound_bank ),
      m_scenemanager_event_dispatcher( scenemanager_event_dispatcher ),
      m_hazard_pushback_target( entt::null )
{
  SPDLOG_DEBUG( "PlayerSystem initialized" );
  std::ignore = get_systems_event_queue().sink<Events::PlayerMortalityEvent>().connect<&PlayerSystem::on_player_mortality_event>( this );
}

void PlayerSystem::update( sf::Time dt )
{
  promote_pending_no_path();

  // cache position so we can update player in spatial grid after changes.
  auto old_player_pos = Utils::Player::get_position( reg() );

  Factory::Particle::delete_expired_particle_sprites( reg(), "graveyard.skele.particle.bones" );

  fade_player_on_wormhole_jump();
  blink_player();

  auto *post_death_timer = reg().try_get<Cmp::Player::PostDeathTimer>( Utils::Player::get_entity( reg() ) );
  if ( post_death_timer ) { *post_death_timer += dt; }
  else
  {
    set_player_on_fire();
    check_player_can_push( dt );
    check_player_can_pull( dt );
    update_player_position( dt );
    update_player_animation();

    Utils::Player::get_zorder( reg() ).setZOrder( Utils::Player::get_position( reg() ).position.y );
  }

  check_player_mortality();
  apply_timed_action_side_effects( dt );
  create_healing_particles();

  if ( is_player_in_light() ) { reg().emplace_or_replace<Cmp::Player::Illuminated>( Utils::Player::get_entity( reg() ) ); }
  else { reg().remove<Cmp::Player::Illuminated>( Utils::Player::get_entity( reg() ) ); }

  if ( PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = m_npc_navmesh.lock() )
  {
    pathfinding_navmesh->update( Utils::Player::get_entity( reg() ), old_player_pos, Utils::Player::get_position( reg() ) );
  }
  if ( PathFinding::SpatialHashGridSharedPtr open_navmesh = m_open_navmesh.lock() )
  {
    open_navmesh->update( Utils::Player::get_entity( reg() ), old_player_pos, Utils::Player::get_position( reg() ) );
  }
}

void PlayerSystem::disable_damage_cooldown()
{
  for ( auto [player_entt, player_cmp] : reg().view<Cmp::Player::Character>().each() )
  {
    player_cmp.m_damage_cooldown_timer.stop();
  }
}

void PlayerSystem::enable_damage_cooldown()
{
  for ( auto [player_entt, player_cmp] : reg().view<Cmp::Player::Character>().each() )
  {
    player_cmp.m_damage_cooldown_timer.restart();
  }
}

void PlayerSystem::force_expire_damage_cooldown()
{
  for ( auto [player_entt, player_cmp] : reg().view<Cmp::Player::Character>().each() )
  {
    player_cmp.skip_damage_cooldown_once = true;
  }
}

bool PlayerSystem::movement_suppressed()
{
  auto *suppress_cmp = reg().try_get<Cmp::Player::MovementSuppressTimer>( Utils::Player::get_entity( reg() ) );
  if ( not suppress_cmp ) return false;

  auto movement_delay = Sys::PersistSystem::get<Cmp::Persist::PostPullMovementDelay>( reg() );
  return suppress_cmp->asSeconds() < movement_delay.get_value();
}

std::optional<Cmp::Direction> PlayerSystem::compute_step_direction( sf::Time dt, bool apply_speed_penalty )
{
  const Cmp::Direction raw_direction = Utils::Player::get_direction( reg() );
  if ( raw_direction == sf::Vector2f( 0.f, 0.f ) ) return std::nullopt; // optimization

  auto &player_movement_speed = Sys::PersistSystem::get<Cmp::Persist::PlayerMovementSpeed>( reg() );
  float speed = player_movement_speed.get_value();
  if ( apply_speed_penalty ) speed *= Utils::Player::get_speed_penalty( reg() );
  const float step = speed * dt.asSeconds();

  return raw_direction.componentWiseMul( { step, step } );
}

void PlayerSystem::move_obstacle( const sf::FloatRect &target_position )
{
  // check if player can move the obstacle
  for ( auto [selected_entt, selected_cmp, moveable_cmp, selected_pos_cmp, selected_uuid_cmp] :
        reg().view<Cmp::SelectedPosition, Cmp::Moveable, Cmp::Position, Cmp::UUID>().each() )
  {
    if ( not target_position.findIntersection( selected_pos_cmp ) ) continue;

    // normalized direction
    auto player_direction = Utils::Player::get_direction( reg() );
    // velocity vector
    auto player_velocity = Cmp::RectBounds::scaled( player_direction.componentWiseMul( Constants::kGridSizePxF ), target_position.size, 1 );
    // where we want the player to end up after the move (used for pull only)
    auto player_dest_position = Cmp::RectBounds::scaled( Utils::Player::get_position( reg() ).position + player_velocity.position(),
                                                         Constants::kGridSizePxF, 1 );
    // where we want the obstacle to end up after the move (push and pull)
    auto obstacle_dest_position = Cmp::RectBounds::scaled( selected_pos_cmp.position + player_velocity.position(), Constants::kGridSizePxF, 1 );

    bool new_position_is_empty = true;
    new_position_is_empty = not Utils::Collision::any_intersects<Cmp::Obstacle>( reg(), obstacle_dest_position ) and
                            not Utils::Collision::any_intersects<Cmp::NoMoveDest>( reg(), obstacle_dest_position );

    // can we move player in the opposite direction if they are pulling?
    bool player_in_the_way = Utils::Collision::any_intersects<Cmp::Player::Character>( reg(), obstacle_dest_position );
    if ( player_in_the_way )
    {
      if ( ( Utils::Collision::any_intersects<Cmp::Obstacle>( reg(), player_dest_position ) ) or
           ( Utils::Collision::any_intersects<Cmp::NoMoveDest>( reg(), player_dest_position ) ) )
      {
        // if not then cancel the move
        new_position_is_empty = false;
      }
    }

    // move the player and the obstacle
    if ( new_position_is_empty )
    {
      Utils::Player::get_position( reg() ).position = player_dest_position.position();

      m_sound_bank.get_effect( "crypt_open" ).play();

      // move the obstacle
      const Cmp::Position vacated_pos = selected_pos_cmp;
      selected_pos_cmp.position += player_velocity.position();

      // keep the NPC navmesh in sync: the cell the block was placed on lost its whole bucket when the navmesh was
      // built (NoPathFinding), so give the vacated cell a floor entity again, and close off the destination cell.
      if ( PathFinding::SpatialHashGridSharedPtr npc_navmesh = m_npc_navmesh.lock() )
      {
        auto vacated_entt = reg().create();
        reg().emplace<Cmp::Position>( vacated_entt, vacated_pos.position, vacated_pos.size );
        npc_navmesh->insert( vacated_entt, vacated_pos );
        npc_navmesh->remove_all( selected_pos_cmp );
      }
      reg().remove<Cmp::SelectedPosition>( selected_entt );

      // find the matching cap to this obstacle and move it too
      for ( auto [cap_search_entt, cap_search_cap_cmp, cap_search_uuid_cmp, cap_search_pos_cmp] :
            reg().view<Cmp::ObstacleCap, Cmp::UUID, Cmp::Position>().each() )
      {
        if ( cap_search_uuid_cmp != selected_uuid_cmp ) continue;
        cap_search_pos_cmp.position += player_velocity.position();
      }

      // if we moved obstacle into cobweb it is now stuck :'(
      for ( auto [cobweb_entt, cobweb_cmp, cobweb_pos_cmp] : reg().view<Cmp::Ruin::Cobweb, Cmp::Position>().each() )
      {
        if ( selected_pos_cmp.findIntersection( cobweb_pos_cmp ) ) { reg().remove<Cmp::Moveable>( selected_entt ); }
      }

      // if we moved obstacle onto rune marking then set it as activated.
      auto rune_view = reg().view<Cmp::RuneMarking, Cmp::Position, Cmp::ZOrderValue, Cmp::AnimData, Cmp::UUID>();
      for ( auto [rune_entt, rune_cmp, rune_pos_cmp, rune_zorder_cmp, rune_anim_cmp, rune_uuid_cmp] : rune_view.each() )
      {
        // Check if ANY obstacle (not just the moved one) is on this rune.
        // Undo the visual y offset so intersection uses the logical rune position —
        // without this, a block sliding up leaves an 8px phantom overlap that
        // prevents deactivation.
        float check_offset = rune_cmp.active ? 8.f : 0.f;
        rune_pos_cmp.position.y += check_offset;

        bool any_obstacle_on_rune = false;
        auto obstacle_view = reg().view<Cmp::Obstacle, Cmp::Position>();
        for ( auto [obs_entt, obstacle_cmp, obstacle_pos_cmp] : obstacle_view.each() )
        {
          if ( obstacle_pos_cmp.findIntersection( rune_pos_cmp ) )
          {
            any_obstacle_on_rune = true;
            break;
          }
        }

        rune_pos_cmp.position.y -= check_offset;

        if ( any_obstacle_on_rune != rune_cmp.active )
        {
          const std::string sprite_type = any_obstacle_on_rune ? "sprite.ruin.runemarking.active" : "sprite.ruin.runemarking.inactive";

          if ( any_obstacle_on_rune )
          {
            Factory::Particle::add_rune_ps( m_reg, "ruin.rune.particle.active", 1.f, 10.f, rune_uuid_cmp,
                                            { rune_pos_cmp.position.x + 8.f, rune_pos_cmp.position.y }, 5000 );
          }
          else
          {
            for ( auto [ps_entt, ps_cmp, ps_uuid_cmp] : reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
            {
              if ( ps_cmp.sprite->get_tag() == "ruin.rune.particle.active" and ps_uuid_cmp == rune_uuid_cmp ) reg().destroy( ps_entt );
            }
          }
          // shift the rune on the y-axis to adjust for the front-facing perspective
          if ( any_obstacle_on_rune ) { rune_pos_cmp.position.y -= 8.f; }
          else { rune_pos_cmp.position.y += 8.f; }

          float zorder = m_sprite_factory.get_spritesheet_by_type( sprite_type ).get_zorder( 0 );
          rune_zorder_cmp.setZOrder( zorder );
          rune_anim_cmp.m_sprite_type = sprite_type;
          rune_cmp.active = any_obstacle_on_rune;
        }
      }
      reg().emplace_or_replace<Cmp::Player::MovementSuppressTimer>( Utils::Player::get_entity( reg() ) );
      break;
    }
  }
}

void PlayerSystem::check_player_can_push( sf::Time dt )
{
  if ( movement_suppressed() ) return;

  auto step_direction = compute_step_direction( dt, false );
  if ( not step_direction ) return;
  const Cmp::Direction &direction = *step_direction;

  const auto player_pos = Utils::Player::get_position( reg() );
  const sf::FloatRect next_horizontal_move( { player_pos.position.x + direction.x, player_pos.position.y }, player_pos.size );
  move_obstacle( next_horizontal_move );
  const sf::FloatRect next_vertical_move( { player_pos.position.x, player_pos.position.y + direction.y }, player_pos.size );
  move_obstacle( next_vertical_move );
}
void PlayerSystem::check_player_can_pull( sf::Time dt )
{
  if ( movement_suppressed() ) return;

  auto step_direction = compute_step_direction( dt, false );
  if ( not step_direction ) return;
  const Cmp::Direction &direction = *step_direction;

  const auto player_pos = Utils::Player::get_position( reg() );
  const sf::FloatRect prev_horizontal_move( { player_pos.position.x - direction.x, player_pos.position.y }, player_pos.size );
  move_obstacle( prev_horizontal_move );
  const sf::FloatRect prev_vertical_move( { player_pos.position.x, player_pos.position.y - direction.y }, player_pos.size );
  move_obstacle( prev_vertical_move );
}

void PlayerSystem::update_player_position( sf::Time dt )
{
  auto *movement_delta = reg().try_get<Cmp::Player::MovementDelta>( Utils::Player::get_entity( reg() ) );
  if ( not movement_delta ) return;
  movement_delta->m_distance = 0.f;

  if ( movement_suppressed() ) return;

  Cmp::Position &player_pos = Utils::Player::get_position( reg() );

  auto step_direction = compute_step_direction( dt, true );
  if ( not step_direction ) return;
  const Cmp::Direction &direction = *step_direction;
  // Direction components are always in {-1, 0, 1} (see Cmp::Direction), so the active axis/axes
  // carry the full step magnitude - recover it here for the edge-nudge clamp below.
  const float step = std::max( std::abs( direction.x ), std::abs( direction.y ) );

  const sf::FloatRect next_horizontal_move( { player_pos.position.x + direction.x, player_pos.position.y }, player_pos.size );
  const sf::FloatRect next_vertical_move( { player_pos.position.x, player_pos.position.y + direction.y }, player_pos.size );

  bool can_move = false;
  bool moved_perp = false;
  Cmp::Direction resolved_dir_vector;

  bool collision_detect_enabled = Utils::scene_setting<Cmp::SceneSettings::CollisionDetection>( reg() ).enabled;

  auto try_horizontal = [&]()
  {
    if ( not collision_detect_enabled or is_valid_move( next_horizontal_move ) )
    {
      can_move = true;
      resolved_dir_vector.x = direction.x;
    }
    else if ( direction.x != 0.0f )
    {
      const float y_offset = Utils::snap_to_grid( player_pos ).position.y - player_pos.position.y;
      const float nudge = std::copysign( std::min( std::abs( y_offset ), step ), y_offset );
      const sf::FloatRect nudged( { player_pos.position.x + direction.x, player_pos.position.y + nudge }, player_pos.size );
      if ( is_valid_move( nudged ) and not moved_perp )
      {
        moved_perp = true;
        can_move = true;
        resolved_dir_vector.x = direction.x;
        resolved_dir_vector.y = nudge;
      }
      else
      {
        // ensure player moves fully against obstacle edge
        player_pos.position.x = Utils::snap_to_grid( player_pos ).position.x;
      }
    }
  };

  auto try_vertical = [&]()
  {
    if ( not collision_detect_enabled or is_valid_move( next_vertical_move ) )
    {
      can_move = true;
      resolved_dir_vector.y = direction.y;
    }
    else if ( direction.y != 0.0f )
    {
      const float x_offset = Utils::snap_to_grid( player_pos ).position.x - player_pos.position.x;
      const float nudge = std::copysign( std::min( std::abs( x_offset ), step ), x_offset );
      const sf::FloatRect nudged( { player_pos.position.x + nudge, player_pos.position.y + direction.y }, player_pos.size );
      if ( is_valid_move( nudged ) and not moved_perp )
      {
        moved_perp = true;
        can_move = true;
        resolved_dir_vector.x = nudge;
        resolved_dir_vector.y = direction.y;
      }
      else
      {
        // else ensure player moves fully against obstacle edge
        player_pos.position.y = Utils::snap_to_grid( player_pos ).position.y;
      }
    }
  };

  // Use the axis of the last committed movement to decide which axis is favored first for the
  // collision detection this frame.
  if ( std::abs( m_last_committed_axis.x ) >= std::abs( m_last_committed_axis.y ) )
  {
    try_horizontal();
    try_vertical();
  }
  else
  {
    try_vertical();
    try_horizontal();
  }

  if ( can_move and collision_detect_enabled and resolved_dir_vector.x != 0.0f and resolved_dir_vector.y != 0.0f )
  {
    // Each axis above is validated independently against its own target tile, so a hazard cell
    // (or wall) that only shares a corner with the player's path is invisible to both checks even
    // though the combined diagonal step lands fully inside it. Re-validate the actual resulting
    // position before committing so a corner-cut can't skip the hazard pushback state machine.
    const sf::FloatRect resolved_target( player_pos.position + resolved_dir_vector, player_pos.size );
    if ( not is_valid_move( resolved_target ) ) can_move = false;
  }

  if ( can_move )
  {
    player_pos.position += resolved_dir_vector;
    movement_delta->m_distance = std::hypot( resolved_dir_vector.x, resolved_dir_vector.y );

    // Record which axis actually carried this frame's movement using the resolved vector (not raw
    // input), purely to bias next frame's axis processing order above - see m_last_committed_axis.
    if ( resolved_dir_vector.x != 0.0f || resolved_dir_vector.y != 0.0f )
    {
      if ( std::abs( resolved_dir_vector.x ) >= std::abs( resolved_dir_vector.y ) )
      {
        m_last_committed_axis = sf::Vector2f{ resolved_dir_vector.x > 0.f ? 1.f : -1.f, 0.f };
      }
      else { m_last_committed_axis = sf::Vector2f{ 0.f, resolved_dir_vector.y > 0.f ? 1.f : -1.f }; }
    }
  }
}

void PlayerSystem::update_player_animation()
{

  if ( movement_suppressed() ) return;

  const Cmp::Direction direction_cmp = Utils::Player::get_direction( reg() );
  Cmp::AnimData &anim_cmp = Utils::Player::get_sprite_anim( reg() );
  const auto *movement_delta = reg().try_get<Cmp::Player::MovementDelta>( Utils::Player::get_entity( reg() ) );
  if ( not movement_delta ) return;

  // Distance the player must cover between animation frame changes; tying the trigger to
  // distance rather than time makes the walk cycle cadence scale with actual movement speed
  // automatically, including any active Cmp::Player::SpeedPenalty (see FootstepSystem for the
  // same pattern applied to footstep sfx).
  const float stride_length = Sys::PersistSystem::get<Cmp::Persist::PlayerAnimStrideLength>( reg() ).get_value();
  const float movement_speed = Sys::PersistSystem::get<Cmp::Persist::PlayerMovementSpeed>( reg() ).get_value();
  const float speed_penalty = Utils::Player::get_speed_penalty( reg() );
  const float effective_speed = movement_speed * speed_penalty;
  anim_cmp.set_framerate( effective_speed > 0.f ? stride_length / effective_speed : stride_length );

  // update the animation state based on movement direction
  if ( direction_cmp == sf::Vector2f( 0.0f, 0.0f ) ) { anim_cmp.m_enabled = false; }
  else
  {
    // Held direction but no real displacement this frame (see PlayerSystem::update_player_position)
    // means collision blocked the move - freeze the walk-cycle instead of animating in place.
    anim_cmp.m_enabled = movement_delta->m_distance > 0.f;
    if ( direction_cmp.x == 1 ) { anim_cmp.m_sprite_type = "sprite.player.walk.east"; }
    else if ( direction_cmp.x == -1 ) { anim_cmp.m_sprite_type = "sprite.player.walk.west"; }
    else if ( direction_cmp.y == -1 ) { anim_cmp.m_sprite_type = "sprite.player.walk.north"; }
    else if ( direction_cmp.y == 1 ) { anim_cmp.m_sprite_type = "sprite.player.walk.south"; }

    // Store the facing direction from raw input - not resolved movement - so it keeps updating even
    // when the player is blocked (e.g. turning to face an obstacle to dig it without moving into it).
    auto &last_dir = Utils::Player::get_last_direction( reg() );
    if ( std::abs( direction_cmp.x ) >= std::abs( direction_cmp.y ) ) { last_dir = sf::Vector2f{ direction_cmp.x > 0.f ? 1.f : -1.f, 0.f }; }
    else { last_dir = sf::Vector2f{ 0.f, direction_cmp.y > 0.f ? 1.f : -1.f }; }
  }
}

void PlayerSystem::check_player_mortality()
{

  auto player_view = reg().view<Cmp::Player::Character, Cmp::Player::Mortality, Cmp::Position>();
  for ( auto [entity, pc_cmp, mortality_cmp, player_pos_cmp] : player_view.each() )
  {
    auto *post_death_timer = reg().try_get<Cmp::Player::PostDeathTimer>( entity );
    if ( post_death_timer and *post_death_timer < post_death_timer->get_timeout() ) continue;
    if ( mortality_cmp.state == Cmp::Player::Mortality::State::DEAD )
    {
      if ( Utils::Player::player_has_extra_life( reg() ) )
      {
        player_pos_cmp.position = Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( reg() );
        Factory::Player::remove_player_extra_life( reg() );
        m_sound_bank.get_effect( "player_respawn" ).play();
        Utils::Player::get_stats( reg() ).apply( { Cmp::Stats::Health{ 100 }, {}, {}, {}, {}, {}, {} } );
        mortality_cmp.state = Cmp::Player::Mortality::State::ALIVE;
        reg().remove<Cmp::NoRender>( entity );
        reg().remove<Cmp::Player::PostDeathTimer>( entity );
      }
      else
      {
        SPDLOG_DEBUG( "Player has progressed to deadness." );
        reg().remove<Cmp::Player::PostDeathTimer>( entity );

        m_scenemanager_event_dispatcher.enqueue<Events::SceneManagerEvent>( Events::SceneManagerEvent::Type::GAME_OVER );
      }
    }
  }
}

void PlayerSystem::apply_timed_action_side_effects( sf::Time dt )
{

  auto &player_stats = Utils::Player::get_stats( reg() );

  const Cmp::BaseAction max_fear_modifier( Cmp::Stats::Health{ -1 }, {}, {}, {}, {}, {}, {} );
  const Cmp::BaseAction max_hypoxia_modifier( Cmp::Stats::Health{ -1 }, {}, {}, {}, {}, {}, {} );
  const Cmp::BaseAction max_tachycardia_modifier( Cmp::Stats::Health{ -1 }, {}, {}, {}, {}, {}, {} );
  const Cmp::BaseAction max_bradycardia_modifier( Cmp::Stats::Health{ -1 }, {}, {}, {}, {}, {}, {} );
  const Cmp::BaseAction max_venom_modifier( Cmp::Stats::Health{ -1 }, {}, {}, {}, {}, {}, {} );
  const Cmp::BaseAction player_onfire_modifier( Cmp::Stats::Health{ -5 }, {}, {}, {}, {}, {} );

  update_timed_action_clocks( dt );

  // Now apply the various stat modifiers if their conditions are met
  static constexpr float kTimedActionSyncClockMax = 1.0;
  m_timed_action_sync_clock += dt;
  if ( m_timed_action_sync_clock.asSeconds() >= kTimedActionSyncClockMax )
  {
    if ( player_stats.toxidrome().at<Cmp::Toxicity::Hypoxia>() == 100 ) { player_stats.apply( max_hypoxia_modifier ); }
    if ( player_stats.toxidrome().at<Cmp::Toxicity::Tachycardia>() == 100 ) { player_stats.apply( max_tachycardia_modifier ); }
    if ( player_stats.toxidrome().at<Cmp::Toxicity::Bradycardia>() == 100 ) { player_stats.apply( max_bradycardia_modifier ); }
    if ( player_stats.toxidrome().at<Cmp::Toxicity::Venom>() == 100 ) { player_stats.apply( max_venom_modifier ); }
    if ( player_stats.fear() == 100 ) { player_stats.apply( max_fear_modifier ); }
    if ( m_player_is_on_fire ) { player_stats.apply( player_onfire_modifier ); }
    apply_npc_modifiers();
    apply_inventory_modifiers();
    apply_healing_spring_modifiers();

    m_timed_action_sync_clock = sf::Time::Zero;

    kill_player_if_max_fear_despair();
  }
}

bool PlayerSystem::is_player_in_light() { return Utils::Collision::is_position_illuminated( reg(), Utils::Player::get_position( reg() ) ); }

void PlayerSystem::apply_healing_spring_modifiers()
{
  // healing spring
  Cmp::BaseAction fountain_effects( Cmp::Stats::Health{ +5 }, Cmp::Stats::Fear{ -5 }, Cmp::Stats::Despair{ -5 }, Cmp::Stats::Infamy{ -5 }, {}, {},
                                    {} );
  for ( auto [fountain_entt, fountain_mb_cmp, fountain_uuid_cmp] : reg().view<Cmp::HealingSpringMultiBlock, Cmp::UUID>().each() )
  {

    if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), fountain_mb_cmp ) ) continue;
    if ( Utils::Player::is_player_near( reg(), fountain_mb_cmp ) ) { reg().emplace_or_replace<Cmp::HealingSpring::ActiveHealing>( fountain_entt ); }
    else { reg().remove<Cmp::HealingSpring::ActiveHealing>( fountain_entt ); }

    float player_distance = Utils::Maths::getEuclideanDistance( fountain_mb_cmp.position, Utils::Player::get_position( reg() ).position );
    if ( player_distance > 500 ) continue;
    Utils::Player::get_stats( reg() ).apply( fountain_effects );

    // Curing toxidromes isn't expressible as a BaseAction delta (it decays/removes existing active
    // toxidromes rather than adding a new one), so it's applied directly here rather than via net_modifier.
    Utils::Player::get_stats( reg() ).decay_all_toxidrome( 5 );
  }
}

void PlayerSystem::apply_npc_modifiers()
{
  // NPC proximity modifiers are only applied when they are within this rectangle
  auto half_view = Cmp::RectBounds::scaled( Utils::calculate_view_bounds( Sys::RenderSystem::get_world_view() ), 0.5f );

  for ( auto [npc_entt, npc_cmp, npc_pos_cmp, npc_anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::AnimData>().each() )
  {
    if ( not Utils::is_visible_in_view( half_view.getBounds(), npc_pos_cmp ) ) continue;
    auto [proximity_action, proximity_timer] = npc_cmp.at<Cmp::ProximityAction>();

    if ( proximity_timer.asSeconds() < proximity_action.interval() ) continue;
    Utils::Player::get_stats( reg() ).apply( proximity_action );
    proximity_timer = sf::Time::Zero;
  }
}

void PlayerSystem::apply_inventory_modifiers()
{
  // We need the inventory component not the types
  for ( auto [slot_entt, slot_cmp] : reg().view<Cmp::PlayerInventorySlot>().each() )
  {
    auto [carry_action, carry_timer] = slot_cmp.m_item.at<Cmp::CarryAction>();
    if ( carry_timer.asSeconds() < carry_action.interval() ) continue;
    Utils::Player::get_stats( reg() ).apply( carry_action );
    carry_timer = sf::Time::Zero;
  }
}

void PlayerSystem::set_player_on_fire()
{
  if ( not Utils::Player::get_stats( reg() ).toxidrome().has<Cmp::Toxicity::Phototoxia>() ) return;

  auto player_pos = Utils::Player::get_position( reg() );
  const sf::Vector2f flame_emitter_pos( player_pos.getCenter().x, player_pos.position.y + player_pos.size.y );

  auto toxicity = Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Phototoxia>();
  if ( toxicity >= 100 )
  {
    for ( auto [ps_owner_entt, ps_owner_cmp] : reg().view<Cmp::Particle::SpriteOwner>().each() )
    {
      if ( ps_owner_cmp.sprite->get_tag() == kPlayerFireTag )
      {
        m_player_is_on_fire = true;
        break;
      }
    }
    if ( not m_player_is_on_fire )
    {
      // set the player on fire
      constexpr auto ps_scale = 0.5f;
      constexpr auto particle_size = 3.f;
      constexpr auto particle_speed = 60.f;
      constexpr auto particle_lifetime = 2.f;
      constexpr auto particle_count = 600;
      auto uuid = Cmp::UUID::generate();
      Factory::Particle::add_flame( reg(), kPlayerFireTag, uuid, flame_emitter_pos, player_pos.position.y + Constants::kGridSizePxF.y, ps_scale,
                                    particle_size, particle_speed, particle_lifetime, particle_count );
    }
    // stll burning
    Factory::Particle::update_position( reg(), kPlayerFireTag, flame_emitter_pos );
    if ( m_sound_bank.get_effect( "burning" ).getStatus() != sf::Sound::Status::Playing ) { m_sound_bank.get_effect( "burning" ).play(); }
    m_sound_bank.get_effect( "burning" ).setLooping( false );
  }
  else
  {
    // put the player fire out
    for ( auto [ps_owner_entt, ps_owner_cmp] : reg().view<Cmp::Particle::SpriteOwner>().each() )
    {
      if ( ps_owner_cmp.sprite->get_tag() == kPlayerFireTag ) { reg().destroy( ps_owner_entt ); }
    }
    m_sound_bank.get_effect( "burning" ).stop();
    m_player_is_on_fire = false;
  }
}

void PlayerSystem::update_timed_action_clocks( sf::Time dt )
{
  auto player_entt = Utils::Player::get_entity( reg() );
  if ( auto *suppress_timer = reg().try_get<Cmp::Player::MovementSuppressTimer>( player_entt ) ) *suppress_timer += dt;

  // update PlayerInventorySlot/NPC/DarknessFear clock every frame.
  for ( auto [slot_entt, slot_cmp] : reg().view<Cmp::PlayerInventorySlot>().each() )
  {
    for ( auto &[action_type, item_action_pair] : slot_cmp.m_item )
    {
      if ( action_type == std::type_index( typeid( Cmp::CollisionAction ) ) ) continue;
      if ( action_type == std::type_index( typeid( Cmp::ProjectileAction ) ) ) continue;
      auto &[item_action, item_action_timer] = item_action_pair;
      if ( item_action.interval() == 0.f ) continue;
      item_action_timer += dt;
      SPDLOG_DEBUG( "PlayerInventorySlot item_action_timer {}", item_action_timer.asSeconds() );
    }
  }
  for ( auto [npc_entt, npc_cmp] : reg().view<Cmp::Npc::NPC>().each() )
  {
    for ( auto &[action_type, npc_action_pair] : npc_cmp )
    {
      if ( action_type == std::type_index( typeid( Cmp::CollisionAction ) ) ) continue;
      if ( action_type == std::type_index( typeid( Cmp::ProjectileAction ) ) ) continue;
      auto &[npc_action, npc_action_timer] = npc_action_pair;
      if ( npc_action.interval() == 0.f ) continue;
      npc_action_timer += dt;
      SPDLOG_DEBUG( "NPC npc_action_timer {}", npc_action_timer.asSeconds() );
    }
  }
  m_darkness_fear_clock += dt;
}

void PlayerSystem::kill_player_if_max_fear_despair()
{
  // dont send mortality event twice during death animation
  if ( Utils::Player::get_mortality( reg() ).state == Cmp::Player::Mortality::State::DEAD ) return;

  // Now, are we dead and if so, what type of death animation should be triggered?
  if ( Utils::Player::get_stats( reg() ).health() == 0 and Utils::Player::get_stats( reg() ).fear() == 100 )
  {
    on_player_mortality_event( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::TERRIFIED, Utils::Player::get_position( reg() ) ) );
  }
  else if ( Utils::Player::get_stats( reg() ).health() == 0 and Utils::Player::get_stats( reg() ).despair() == 100 )
  {
    on_player_mortality_event( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::SUICIDE, Utils::Player::get_position( reg() ) ) );
  }
  else if ( Utils::Player::get_stats( reg() ).health() == 0 and Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Phototoxia>() == 100 )
  {
    m_player_is_on_fire = false;
    on_player_mortality_event( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::IGNITED, Utils::Player::get_position( reg() ) ) );
  }
}

void PlayerSystem::create_healing_particles()
{
  for ( auto [fountain_entt, fountain_mb_cmp, fountain_uuid_cmp] : reg().view<Cmp::HealingSpringMultiBlock, Cmp::UUID>().each() )
  {
    if ( not reg().any_of<Cmp::HealingSpring::ActiveHealing>( fountain_entt ) ) continue;

    // Add player healing particle sprite if player is near fountain and doesn't already have PS already, otherwise delete any existing PS.
    std::vector<entt::entity> player_healing_ps_owner_list;
    for ( auto [ps_entt, ps_cmp, ps_uuid_cmp] : reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
    {
      if ( ps_uuid_cmp != fountain_uuid_cmp ) continue;
      if ( ps_cmp.sprite->get_tag() == "player.healing.particle.active" ) player_healing_ps_owner_list.push_back( ps_entt );
    }
    if ( Utils::Player::is_player_near( reg(), fountain_mb_cmp ) )
    {
      if ( player_healing_ps_owner_list.empty() )
      {
        auto player_pos = Utils::Player::get_position( reg() );
        Factory::Particle::add_player_healing_ps( reg(), "player.healing.particle.active", 0.3F, 10.f, fountain_uuid_cmp, player_pos.getCenter(),
                                                  player_pos.y() - 1 );
      }
      Factory::Particle::update_position( reg(), fountain_uuid_cmp, Utils::Player::get_position( reg() ).getCenter() );
    }
    else
    {
      for ( auto ps_entt : player_healing_ps_owner_list )
      {
        reg().destroy( ps_entt );
      }
    }
  }
}

void PlayerSystem::fade_player_on_wormhole_jump()
{
  auto player_entt = Utils::Player::get_entity( reg() );
  auto *wormhole_jump = reg().try_get<Cmp::Wormhole::Jump>( player_entt );
  if ( wormhole_jump )
  {
    // Calculate fade based on elapsed time vs total cooldown
    float elapsed = wormhole_jump->jump_clock.getElapsedTime().asSeconds();
    float cooldown = wormhole_jump->jump_cooldown.asSeconds();
    float progress = std::min( elapsed / cooldown, 1.0f ); // 0.0 to 1.0
    Utils::Player::get_alpha( reg() ).setAlpha( static_cast<uint8_t>( 255 * ( 1.0f - progress ) ) );
  }
}

void PlayerSystem::blink_player()
{
  // damage cooldown blink effect
  for ( auto [player_entt, player_cmp] : reg().view<Cmp::Player::Character>().each() )
  {
    // fade_player_on_wormhole_jump() owns alpha while a jump is pending - don't clobber its fade
    if ( reg().all_of<Cmp::Wormhole::Jump>( player_entt ) ) continue;

    auto &pc_damage_cooldown = Sys::PersistSystem::get<Cmp::Persist::PcDamageDelay>( reg() );
    bool is_in_damage_cooldown = player_cmp.m_damage_cooldown_timer.getElapsedTime().asSeconds() < pc_damage_cooldown.get_value();
    bool blink_visible = ( player_cmp.m_damage_cooldown_timer.getElapsedTime().asMilliseconds() / 100 ) % 2 == 0;

    auto &alpha_cmp = Utils::Player::get_alpha( reg() );
    if ( not is_in_damage_cooldown or player_cmp.skip_damage_cooldown_once or ( is_in_damage_cooldown and blink_visible ) ) { alpha_cmp = 255; }
    else { alpha_cmp = 0; }
  }
}

bool PlayerSystem::is_valid_move( const sf::FloatRect &target_position )
{
  auto search_bounds = Cmp::RectBounds::scaled( target_position.position, target_position.size, 1 );

  // Use the spatial hash to check only the ~5 adjacent cells instead of scanning all
  // NoPath entities (walls + obstacles). Without this, the open spawn area causes
  // a full O(N) scan every frame because no entity triggers an early exit.
  if ( auto navmesh = m_player_navmesh.lock() )
  {
    Cmp::Position target_pos( target_position.position, target_position.size );
    for ( auto candidate_entt : navmesh->neighbours( target_pos, PathFinding::QueryCompass::BOTH ) )
    {
      if ( not reg().all_of<Cmp::Player::NoPath>( candidate_entt ) ) continue;
      auto *pos_cmp = reg().try_get<Cmp::Position>( candidate_entt );
      if ( not pos_cmp ) continue;
      if ( search_bounds.findIntersection( *pos_cmp ) ) return false;
    }
  }
  else
  {
    for ( auto [candidate_entt, candidate_pos] : reg().view<Cmp::Player::NoPath, Cmp::Position>().each() )
    {
      if ( search_bounds.findIntersection( candidate_pos ) ) return false;
    }
  }

  // update_player_position() calls is_valid_move() for both axes every frame, even the axis with
  // zero direction (its target is just the player's current tile). Skip the hazard pushback state
  // machine for that no-op axis, otherwise it resets the timer being accumulated by the other axis's
  // real approach on every single frame, so the player can never push through.
  if ( target_position.position == Utils::Player::get_position( reg() ).position ) return true;

  return resolve_hazard_pushback( search_bounds );
}

bool PlayerSystem::resolve_hazard_pushback( const Cmp::RectBounds &search_bounds )
{
  entt::entity hazard_entt = entt::null;
  for ( auto [candidate_entt, resist_cmp, pos_cmp] : reg().view<Cmp::Hazard::CollisionResist, Cmp::Position>().each() )
  {
    if ( search_bounds.findIntersection( pos_cmp ) )
    {
      hazard_entt = candidate_entt;
      break;
    }
  }

  if ( hazard_entt == entt::null )
  {
    m_hazard_pushback_target = entt::null;
    return true;
  }

  if ( hazard_entt != m_hazard_pushback_target )
  {
    m_hazard_pushback_target = hazard_entt;
    m_hazard_pushback_clock.restart();
    return false;
  }

  const auto &resist_cmp = reg().get<Cmp::Hazard::CollisionResist>( hazard_entt );
  return m_hazard_pushback_clock.getElapsedTime().asSeconds() >= resist_cmp.resist_seconds;
}

void PlayerSystem::promote_pending_no_path()
{
  auto player_pos = Utils::Player::get_position( reg() );
  for ( auto [seg_entt, seg_pos_cmp] : reg().view<Cmp::Player::PendingNoPath, Cmp::Position>().each() )
  {
    if ( player_pos.findIntersection( seg_pos_cmp ) ) continue; // still standing on it

    reg().remove<Cmp::Player::PendingNoPath>( seg_entt );
    reg().emplace_or_replace<Cmp::Player::NoPath>( seg_entt );
    reg().emplace_or_replace<Cmp::Particle::BlockParticle>( seg_entt );

    // This entity was never indexed while pending (create_player_navmesh only sees entities that
    // already have Cmp::Player::NoPath), so insert it now rather than waiting for a rebuild.
    if ( auto player_navmesh = m_player_navmesh.lock() ) { player_navmesh->insert( seg_entt, seg_pos_cmp ); }
  }
}

void PlayerSystem::on_player_mortality_event( Game::Events::PlayerMortalityEvent ev )
{

  auto common_death_throes = [&]()
  {
    if ( auto *movement_delta = reg().try_get<Cmp::Player::MovementDelta>( Utils::Player::get_entity( reg() ) ) )
    {
      movement_delta->m_distance = 0; // stop footstep sfx
    }
    reg().emplace_or_replace<Cmp::Player::PostDeathTimer>( Utils::Player::get_entity( reg() ) );
    reg().emplace_or_replace<Cmp::NoRender>( Utils::Player::get_entity( reg() ) );
    Utils::Player::get_stats( reg() ).apply( { Cmp::Stats::Health{ -100 }, {}, {}, {}, {}, {} } );
    SPDLOG_INFO( "Player death code: {}", static_cast<uint8_t>( ev.m_new_state ) );
    Utils::Player::get_mortality( reg() ).state = Cmp::Player::Mortality::State::DEAD;
    SPDLOG_INFO( "Player died" );
  };

  switch ( ev.m_new_state )
  {
    case Cmp::Player::Mortality::State::ALIVE:
      break;

    case Cmp::Player::Mortality::State::FALLING: {
      SPDLOG_INFO( "Player is falling" );
      const auto &sprite = m_sprite_factory.get_spritesheet_by_type( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::DECAYING: {
      const auto &sprite = m_sprite_factory.get_spritesheet_by_type( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::HAUNTED: {
      const auto &sprite = m_sprite_factory.get_spritesheet_by_type( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::EXPLODING: {
      const auto &sprite = m_sprite_factory.get_spritesheet_by_type( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::DROWNING: {
      break;
    }
    case Cmp::Player::Mortality::State::SQUISHED: {
      const auto &sprite = m_sprite_factory.get_spritesheet_by_type( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::SUICIDE: {
      const auto &sprite = m_sprite_factory.get_spritesheet_by_type( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::IGNITED: {
      const auto &sprite = m_sprite_factory.get_spritesheet_by_type( "sprite.death.anim.flames" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "shrine_lighting" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::SKEWERED: {
      const auto &sprite = m_sprite_factory.get_spritesheet_by_type( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::SHOCKED: {
      const auto &sprite = m_sprite_factory.get_spritesheet_by_type( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::TERRIFIED: {
      const auto &sprite = m_sprite_factory.get_spritesheet_by_type( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::DEAD: {
      break;
    }
    case Cmp::Player::Mortality::State::SHADOWCURSED:
      const auto &sprite = m_sprite_factory.get_spritesheet_by_type( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
  }
}

} // namespace Game::Sys