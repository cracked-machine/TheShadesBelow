#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteAlpha.hpp>
#include <Components/AbsoluteOffset.hpp>
#include <Components/AbsoluteRenderOffset.hpp>
#include <Components/AbsoluteRotation.hpp>
#include <Components/AbsoluteTint.hpp>
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
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/StingDamage.hpp>
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
#include <Components/Particle/EmberParticleSprite.hpp>
#include <Components/Particle/LavaFireballParticleSprite.hpp>
#include <Components/Particle/LightningRodParticleSprite.hpp>
#include <Components/Particle/NoteParticleSprite.hpp>
#include <Components/Particle/SpriteBase.hpp>
#include <Components/Particle/SpriteOwner.hpp>
#include <Components/PendingTeleport.hpp>
#include <Components/Persistent/AzazelFireballDamage.hpp>
#include <Components/Persistent/BradycardiaHealthDmgPerSec.hpp>
#include <Components/Persistent/DespairHealthDmgPerSec.hpp>
#include <Components/Persistent/EmberDamage.hpp>
#include <Components/Persistent/FearHealthDmgPerSec.hpp>
#include <Components/Persistent/HallucinogenHealthDmgPerSec.hpp>
#include <Components/Persistent/HypoxiaHealthDmgPerSec.hpp>
#include <Components/Persistent/LavaFireballDamage.hpp>
#include <Components/Persistent/LightningRodDebrisDamage.hpp>
#include <Components/Persistent/LuckDropRate.hpp>
#include <Components/Persistent/PlayerAnimStrideLength.hpp>
#include <Components/Persistent/PlayerCornerAssistTolerance.hpp>
#include <Components/Persistent/PlayerDiagonalGapCatch.hpp>
#include <Components/Persistent/PlayerDiagonalLerpSpeedModifier.hpp>
#include <Components/Persistent/PlayerOnFireHealthDmgPerSec.hpp>
#include <Components/Persistent/PlayerRenderOffset.hpp>
#include <Components/Persistent/PlayerShortcutLerpSpeedModifier.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Persistent/PostPullMovementDelay.hpp>
#include <Components/Persistent/RepelNpcPlayerTint.hpp>
#include <Components/Persistent/TachycardiaHealthDmgPerSec.hpp>
#include <Components/Persistent/VenomHealthDmgPerSec.hpp>
#include <Components/Persistent/VertigoHealthDmgPerSec.hpp>
#include <Components/Plant/BurningTimer.hpp>
#include <Components/Player/ArrowCompass.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/DiggingTimer.hpp>
#include <Components/Hazard/SinkholeCell.hpp>
#include <Components/Persistent/HazardBalanceOverlap.hpp>
#include <Components/Persistent/HazardBalanceTimeout.hpp>
#include <Components/Player/HazardBalance.hpp>
#include <Components/Player/HeartBeat.hpp>
#include <Components/Player/Illuminated.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Player/MovementDelta.hpp>
#include <Components/Player/MovementSuppressTimer.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Player/PendingNoPath.hpp>
#include <Components/Player/PostDeathTimeout.hpp>
#include <Components/Player/RepelNpc.hpp>
#include <Components/Player/TookDamage.hpp>
#include <Components/Player/TorchRadius.hpp>
#include <Components/Player/ToxicImmunity.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Ruin/Cobweb.hpp>
#include <Components/Ruin/RuneMarking.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/SelectedPosition.hpp>
#include <Components/Stats/ActionReward.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/PlayerStats.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/SideEffects.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Components/Toxicity/Venom.hpp>
#include <Components/Toxicity/Vertigo.hpp>
#include <Components/UUID.hpp>
#include <Components/Wall.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/CreateLootEvent.hpp>
#include <Events/DestroyPlantEvent.hpp>
#include <Events/GrimoireItemUpdateEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <Events/UpdatePlayerPositionEvent.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <Systems/FlyingSystem.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PlayerSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/Cardinal.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Crypt.hpp>
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
#include <array>
#include <cmath>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <typeindex>

namespace Game::Sys
{

PlayerSystem::PlayerSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank,
                            entt::dispatcher &scenemanager_event_dispatcher )
    : BaseSystem( reg, window, sound_bank ),
      m_scenemanager_event_dispatcher( scenemanager_event_dispatcher )
{
  SPDLOG_DEBUG( "PlayerSystem initialized" );
  std::ignore = get_systems_event_queue().sink<Events::PlayerMortalityEvent>().connect<&PlayerSystem::on_player_mortality_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::UpdatePlayerPositionEvent>().connect<&PlayerSystem::on_update_player_position_event>( this );
}

void PlayerSystem::on_update_player_position_event( const Events::UpdatePlayerPositionEvent &event )
{
  auto &player_pos = reg().get<Cmp::Position>( Utils::Player::get_entity( reg() ) );
  const sf::Vector2f old_position = m_last_written_pos.value_or( player_pos.position );
  player_pos.position = event.m_position;
  sync_player_navmesh( old_position );
}

void PlayerSystem::sync_player_navmesh( sf::Vector2f old_position )
{
  const auto player_entt = Utils::Player::get_entity( reg() );
  const Cmp::Position &player_pos = Utils::Player::get_position( reg() );
  const Cmp::Position old_player_pos( old_position, player_pos.size );

  if ( PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = m_npc_navmesh.lock() )
  {
    pathfinding_navmesh->update( player_entt, old_player_pos, player_pos );
  }
  if ( PathFinding::SpatialHashGridSharedPtr open_navmesh = m_open_navmesh.lock() )
  {
    open_navmesh->update( player_entt, old_player_pos, player_pos );
  }
  m_last_written_pos = player_pos.position;
}

void PlayerSystem::update( sf::Time dt )
{
  promote_pending_no_path();

  // The navmeshes hold the player at the last position written by on_update_player_position_event. If the
  // component has moved since then, something wrote it directly and left the navmeshes behind: report it and resync.
  const sf::Vector2f current_player_pos = Utils::Player::get_position( reg() ).position;
  if ( not m_last_written_pos ) { m_last_written_pos = current_player_pos; }
  else if ( *m_last_written_pos != current_player_pos )
  {
    SPDLOG_ERROR( "Player Cmp::Position was modified directly ({},{} -> {},{}): trigger Events::UpdatePlayerPositionEvent instead",
                  m_last_written_pos->x, m_last_written_pos->y, current_player_pos.x, current_player_pos.y );
    sync_player_navmesh( *m_last_written_pos );
  }

  Factory::Particle::delete_expired_particle_sprites( reg(), "graveyard.skele.particle.bones" );

  for ( auto [player_entt, player_cmp] : reg().view<Cmp::Player::Character>().each() )
  {
    if ( not player_cmp.damage_cooldown_paused ) { player_cmp.m_damage_cooldown_timer += dt; }
  }

  blink_player();

  // resting render offset, refreshed every frame so tuning applies immediately in every scene.
  // FlyingSystem owns the offset while flying (it adds the lift on top of this)
  if ( not Utils::Player::is_flying( reg() ) )
  {
    const float render_offset_y = Sys::PersistSystem::get<Cmp::Persist::PlayerRenderOffset>( reg() ).get_value();
    reg().emplace_or_replace<Cmp::AbsoluteRenderOffset>( Utils::Player::get_entity( reg() ), sf::Vector2f{ 0.f, render_offset_y } );
  }

  auto *post_death_timer = reg().try_get<Cmp::Player::PostDeathTimer>( Utils::Player::get_entity( reg() ) );
  if ( post_death_timer ) { *post_death_timer += dt; }
  else
  {
    set_player_on_fire();
    check_burning_particle_collision( Cmp::Particle::LavaFireballParticleSprite::kTagSuffix,
                                      Sys::PersistSystem::get<Cmp::Persist::LavaFireballDamage>( reg() ).get_value() );
    check_burning_particle_collision( Cmp::Particle::LavaFireballParticleSprite::kAzazelTagSuffix,
                                      Sys::PersistSystem::get<Cmp::Persist::AzazelFireballDamage>( reg() ).get_value() );
    check_burning_particle_collision( Cmp::Particle::EmberParticleSprite::kTagSuffix,
                                      Sys::PersistSystem::get<Cmp::Persist::EmberDamage>( reg() ).get_value() );
    check_burning_particle_collision( Cmp::Particle::LightningRodParticleSprite::kTagSuffix,
                                      Sys::PersistSystem::get<Cmp::Persist::LightningRodDebrisDamage>( reg() ).get_value(),
                                      Cmp::Player::Mortality::State::SHOCKED );
    update_luck_drop( dt );
    check_player_can_push( dt );
    check_player_can_pull( dt );
    // FlyingSystem owns the player position while descending to the landing cell, and a taking-off player
    // can't move until they have risen above the obstacle (1x1 cell) height
    const bool grounded_by_flight = Utils::Player::is_landing( reg() ) or
                                    ( Utils::Player::is_flying( reg() ) and not Utils::Player::is_flying_elevated( reg() ) );
    const bool balancing = update_hazard_balance( dt );
    if ( balancing )
    {
      if ( auto *movement_delta = reg().try_get<Cmp::Player::MovementDelta>( Utils::Player::get_entity( reg() ) ) )
      {
        movement_delta->m_distance = 0.f; // no footsteps while frozen
      }
    }
    else if ( not grounded_by_flight ) { update_player_position( dt ); }
    update_player_animation();

    // once the flying player's sprite clears the obstacle height it is drawn in front of obstacles, except
    // walls and sprites with an explicit spritesheets.json zorder (see FlyingSystem::elevated_zorder)
    float player_zorder = Utils::Player::get_position( reg() ).position.y;
    if ( Utils::Player::is_flying_elevated( reg() ) ) { player_zorder = FlyingSystem::elevated_zorder( reg() ); }
    else if ( const auto *balance_cmp = reg().try_get<Cmp::Player::HazardBalance>( Utils::Player::get_entity( reg() ) ) )
    {
      // leaning out over a hazard cell below (south) would otherwise be drawn behind its sprite
      if ( reg().valid( balance_cmp->hazard ) )
      {
        const auto *hazard_z = reg().try_get<Cmp::ZOrderValue>( balance_cmp->hazard );
        const auto *hazard_pos = reg().try_get<Cmp::Position>( balance_cmp->hazard );
        const float hazard_zorder = hazard_z ? hazard_z->get() : ( hazard_pos ? hazard_pos->position.y : player_zorder );
        player_zorder = std::max( player_zorder, hazard_zorder + 1.f );
      }
    }
    Utils::Player::get_zorder( reg() ).set( player_zorder );
  }

  check_player_mortality();
  apply_timed_action_side_effects( dt );
  update_repel_npc( dt );
  update_toxic_immunity( dt );
  update_note_particles();

  update_heartbeat( dt );

  if ( auto *compass_cmp = reg().try_get<Cmp::Player::ArrowCompass>( Utils::Player::get_entity( reg() ) ) )
  {
    compass_cmp->m_retarget_timer += dt;
    compass_cmp->m_osc_timer += dt;
  }

  Utils::Player::get_global_bomb_flash_timer( reg() ) += dt;

  if ( is_player_in_light() ) { reg().emplace_or_replace<Cmp::Player::Illuminated>( Utils::Player::get_entity( reg() ) ); }
  else { reg().remove<Cmp::Player::Illuminated>( Utils::Player::get_entity( reg() ) ); }
}

void PlayerSystem::disable_damage_cooldown()
{
  for ( auto [player_entt, player_cmp] : reg().view<Cmp::Player::Character>().each() )
  {
    player_cmp.damage_cooldown_paused = true;
  }
}

void PlayerSystem::enable_damage_cooldown()
{
  for ( auto [player_entt, player_cmp] : reg().view<Cmp::Player::Character>().each() )
  {
    player_cmp.damage_cooldown_paused = false;
    player_cmp.m_damage_cooldown_timer = sf::Time::Zero;
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

  const auto &movement_delay = Sys::PersistSystem::get<Cmp::Persist::PostPullMovementDelay>( reg() );
  return suppress_cmp->asSeconds() < movement_delay.get_value();
}

std::optional<Cmp::Direction> PlayerSystem::compute_step_direction( sf::Time dt, bool apply_speed_penalty )
{
  const Cmp::Direction raw_direction = Utils::Player::get_direction( reg() );
  if ( raw_direction == sf::Vector2f( 0.f, 0.f ) ) return std::nullopt; // optimization

  float speed = Utils::Player::get_movement_speed( reg() );
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
    const auto &player_direction = Utils::Player::get_direction( reg() );
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
      on_update_player_position_event( Events::UpdatePlayerPositionEvent( player_dest_position.position() ) );

      m_sound_bank.get_effect( "crypt_open" ).play();

      // move the obstacle
      const Cmp::Position vacated_pos = selected_pos_cmp;
      selected_pos_cmp.position += player_velocity.position();
      if ( auto *zorder_cmp = reg().try_get<Cmp::ZOrderValue>( selected_entt ) )
      {
        zorder_cmp->set( zorder_cmp->get() + player_velocity.position().y );
      }

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
        if ( auto *zorder_cmp = reg().try_get<Cmp::ZOrderValue>( cap_search_entt ) )
        {
          zorder_cmp->set( zorder_cmp->get() + player_velocity.position().y );
        }
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
          const Sys::SpriteKey sprite_type = any_obstacle_on_rune ? "sprite.ruin.runemarking.active" : "sprite.ruin.runemarking.inactive";

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

          float zorder = Sys::SpriteStore::instance().get( sprite_type ).zorder( 0, rune_pos_cmp.position.y );
          rune_zorder_cmp.set( zorder );
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

  const Cmp::Position &player_pos = Utils::Player::get_position( reg() );

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

  // Max perpendicular misalignment that the corner assist will correct (0 disables it).
  const float corner_assist_tolerance = Sys::PersistSystem::get<Cmp::Persist::PlayerCornerAssistTolerance>( reg() ).get_value();

  // Corner assist: the player hitbox is exactly one grid cell, so a 1-cell gap only admits the player at one
  // exact perpendicular offset, which continuous movement steps rarely land on. When the forward move along
  // `axis` is blocked, check whether the same move would be valid from the nearest grid-aligned row/column;
  // if so (and the misalignment is within tolerance) slide perpendicular towards alignment this frame instead.
  // The forward move then succeeds on a later frame once aligned. Probing from the aligned position means this
  // only triggers when there really is an opening, so sliding along a solid wall never drifts sideways.
  // Returns true if a perpendicular nudge was applied.
  auto try_corner_assist = [&]( bool horizontal ) -> bool
  {
    if ( moved_perp or corner_assist_tolerance <= 0.f ) return false;
    // another axis is already moving freely this frame (diagonal input sliding along a wall): don't fight it
    if ( horizontal ? resolved_dir_vector.y != 0.f : resolved_dir_vector.x != 0.f ) return false;

    const sf::Vector2f snapped = Utils::snap_to_grid( player_pos ).position;
    const float offset = horizontal ? snapped.y - player_pos.position.y : snapped.x - player_pos.position.x;
    if ( offset == 0.f or std::abs( offset ) > corner_assist_tolerance ) return false;

    const sf::FloatRect aligned_move = horizontal
                                           ? sf::FloatRect( { player_pos.position.x + direction.x, snapped.y }, player_pos.size )
                                           : sf::FloatRect( { snapped.x, player_pos.position.y + direction.y }, player_pos.size );
    if ( not is_valid_move( aligned_move ) ) return false;

    const float nudge = std::copysign( std::min( std::abs( offset ), step ), offset );
    if ( horizontal ) { resolved_dir_vector.y = nudge; }
    else { resolved_dir_vector.x = nudge; }
    moved_perp = true;
    can_move = true;
    return true;
  };

  // set once a blocked move starts the hazard balance window: the player is frozen from then on
  auto balancing = [&]() { return reg().all_of<Cmp::Player::HazardBalance>( Utils::Player::get_entity( reg() ) ); };

  auto try_horizontal = [&]()
  {
    if ( balancing() ) return;
    // No horizontal input: nothing to resolve. Must not touch resolved_dir_vector here, otherwise a
    // perpendicular nudge applied by the vertical axis' corner assist would be overwritten.
    if ( direction.x == 0.0f ) return;

    if ( not collision_detect_enabled or is_valid_move( next_horizontal_move ) )
    {
      can_move = true;
      resolved_dir_vector.x = direction.x;
    }
    else if ( const auto hazard_entt = Utils::Collision::find_lethal_hazard( reg(), Utils::Collision::lethal_hazard_probe( reg(), next_horizontal_move ) );
              hazard_entt != entt::null )
    {
      // blocked by a lethal hazard: balance half over its edge
      begin_hazard_balance( hazard_entt, Utils::Cardinal::dominant( { direction.x, 0.f } ) );
    }
    else if ( not try_corner_assist( true ) and resolved_dir_vector.x == 0.0f )
    {
      // ensure player moves fully against obstacle edge (skipped if x already carries a corner-assist nudge)
      on_update_player_position_event(
          Events::UpdatePlayerPositionEvent( { Utils::snap_to_grid( player_pos ).position.x, player_pos.position.y } ) );
    }
  };

  auto try_vertical = [&]()
  {
    if ( balancing() ) return;
    // No vertical input: see try_horizontal.
    if ( direction.y == 0.0f ) return;

    if ( not collision_detect_enabled or is_valid_move( next_vertical_move ) )
    {
      can_move = true;
      resolved_dir_vector.y = direction.y;
    }
    else if ( const auto hazard_entt = Utils::Collision::find_lethal_hazard( reg(), Utils::Collision::lethal_hazard_probe( reg(), next_vertical_move ) );
              hazard_entt != entt::null )
    {
      // blocked by a lethal hazard: balance half over its edge
      begin_hazard_balance( hazard_entt, Utils::Cardinal::dominant( { 0.f, direction.y } ) );
    }
    else if ( not try_corner_assist( false ) and resolved_dir_vector.y == 0.0f )
    {
      // ensure player moves fully against obstacle edge (skipped if y already carries a corner-assist nudge)
      on_update_player_position_event(
          Events::UpdatePlayerPositionEvent( { player_pos.position.x, Utils::snap_to_grid( player_pos ).position.y } ) );
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

  if ( balancing() ) return;

  // Diagonal gap catch: with two perpendicular directions held, one axis slides freely along a wall while the
  // other is blocked. Without this, catching a 1-cell opening in that wall depends on a frame happening to land
  // within the sub-pixel window where the player fits, so at speed the slide frequently steps straight past it.
  // If this frame's slide would reach or cross the next grid-aligned position, and the blocked axis' move is
  // valid from there (i.e. there is an opening), clamp the slide to stop exactly aligned with the opening.
  // Next frame both axes pass individually but the combined diagonal step doesn't (see below), so the player
  // holds at the opening until one key is released.
  if ( can_move and collision_detect_enabled and direction.x != 0.0f and direction.y != 0.0f and
       Sys::PersistSystem::get<Cmp::Persist::PlayerDiagonalGapCatch>( reg() ).get_value() )
  {
    auto catch_gap = [&]( bool slide_horizontal )
    {
      const float grid = Constants::kGridSizePxF.x;
      const float pos = slide_horizontal ? player_pos.position.x : player_pos.position.y;
      const float slide = slide_horizontal ? resolved_dir_vector.x : resolved_dir_vector.y;

      // next grid line strictly ahead in the slide direction (an aligned player whose blocked axis still
      // failed has no opening here, so look for the next one)
      const float next_line = slide > 0.f ? ( std::floor( pos / grid ) + 1.f ) * grid : ( std::ceil( pos / grid ) - 1.f ) * grid;
      if ( std::abs( next_line - pos ) > std::abs( slide ) ) return;

      const sf::FloatRect opening_probe = slide_horizontal
                                              ? sf::FloatRect( { next_line, player_pos.position.y + direction.y }, player_pos.size )
                                              : sf::FloatRect( { player_pos.position.x + direction.x, next_line }, player_pos.size );
      if ( not is_valid_move( opening_probe ) ) return;

      if ( slide_horizontal ) { resolved_dir_vector.x = next_line - pos; }
      else { resolved_dir_vector.y = next_line - pos; }
    };

    // exactly one axis moving freely means the other one was blocked
    if ( resolved_dir_vector.x != 0.0f and resolved_dir_vector.y == 0.0f ) { catch_gap( true ); }
    else if ( resolved_dir_vector.y != 0.0f and resolved_dir_vector.x == 0.0f ) { catch_gap( false ); }
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
    on_update_player_position_event( Events::UpdatePlayerPositionEvent( player_pos.position + resolved_dir_vector ) );
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
  const float movement_speed = Utils::Player::get_movement_speed( reg() );
  const float speed_penalty = Utils::Player::get_speed_penalty( reg() );
  const float effective_speed = movement_speed * speed_penalty;
  anim_cmp.set_framerate_override( effective_speed > 0.f ? effective_speed / stride_length : 1.f / stride_length );

  auto &last_dir = Utils::Player::get_last_direction( reg() );
  const bool flying = Utils::Player::is_flying( reg() );

  // balancing on a lethal hazard's edge: wobble towards it at the sheet's own framerate
  if ( const auto *balance_cmp = reg().try_get<Cmp::Player::HazardBalance>( Utils::Player::get_entity( reg() ) ) )
  {
    // indexed by Utils::Cardinal::Value: East, South, West, North
    static constexpr std::array<std::string_view, Utils::Cardinal::Count> kWobbleSprites{
        "sprite.player.wobble.east", "sprite.player.wobble.south", "sprite.player.wobble.west", "sprite.player.wobble.north" };
    anim_cmp.m_sprite_type = Sys::SpriteKey( std::string( kWobbleSprites[balance_cmp->hazard_cell_dir.value()] ) );
    anim_cmp.m_enabled = true;
    anim_cmp.clear_framerate_override();
    last_dir = balance_cmp->hazard_cell_dir.vector();
    return;
  }

  // update the animation state based on movement direction
  if ( direction_cmp == sf::Vector2f( 0.0f, 0.0f ) ) { anim_cmp.m_enabled = false; }
  else
  {
    // Held direction but no real displacement this frame (see PlayerSystem::update_player_position)
    // means collision blocked the move - freeze the walk-cycle instead of animating in place.
    anim_cmp.m_enabled = movement_delta->m_distance > 0.f;
    if ( not flying )
    {
      if ( direction_cmp.x == 1 ) { anim_cmp.m_sprite_type = "sprite.player.walk.east"; }
      else if ( direction_cmp.x == -1 ) { anim_cmp.m_sprite_type = "sprite.player.walk.west"; }
      else if ( direction_cmp.y == -1 ) { anim_cmp.m_sprite_type = "sprite.player.walk.north"; }
      else if ( direction_cmp.y == 1 ) { anim_cmp.m_sprite_type = "sprite.player.walk.south"; }
    }

    // Store the facing direction from raw input - not resolved movement - so it keeps updating even
    // when the player is blocked (e.g. turning to face an obstacle to dig it without moving into it).
    if ( std::abs( direction_cmp.x ) >= std::abs( direction_cmp.y ) ) { last_dir = sf::Vector2f{ direction_cmp.x > 0.f ? 1.f : -1.f, 0.f }; }
    else { last_dir = sf::Vector2f{ 0.f, direction_cmp.y > 0.f ? 1.f : -1.f }; }
  }

  // The flying sheet is a single east-facing sequence: mirror it for west, keep the last side for north/south.
  // It animates continuously at the sheet's own framerate, even while hovering or blocked.
  if ( flying )
  {
    anim_cmp.m_sprite_type = "sprite.player.flying";
    anim_cmp.m_enabled = true;
    anim_cmp.clear_framerate_override();
    if ( last_dir.x > 0.f ) { anim_cmp.m_flip_x = false; }
    else if ( last_dir.x < 0.f ) { anim_cmp.m_flip_x = true; }
  }
  // just landed, or just stopped balancing: restore the walking sprite for the current facing, even if the
  // player is standing still
  else if ( anim_cmp.m_sprite_type == "sprite.player.flying" or anim_cmp.m_sprite_type.contains( "sprite.player.wobble" ) )
  {
    restore_walk_sprite( anim_cmp, last_dir );
  }
}

void PlayerSystem::restore_walk_sprite( Cmp::AnimData &anim_cmp, sf::Vector2f last_dir )
{
  // the flying/wobble sheets don't share the walk sheets' frame count, so restart the walk cycle to stay in range
  anim_cmp.m_flip_x = false;
  anim_cmp.m_current_frame = 0;
  if ( last_dir.x > 0.f ) { anim_cmp.m_sprite_type = "sprite.player.walk.east"; }
  else if ( last_dir.x < 0.f ) { anim_cmp.m_sprite_type = "sprite.player.walk.west"; }
  else if ( last_dir.y < 0.f ) { anim_cmp.m_sprite_type = "sprite.player.walk.north"; }
  else { anim_cmp.m_sprite_type = "sprite.player.walk.south"; }
}

void PlayerSystem::check_player_mortality()
{

  auto player_view = reg().view<Cmp::Player::Character, Cmp::Player::Mortality, const Cmp::Position>();
  for ( auto [entity, pc_cmp, mortality_cmp, player_pos_cmp] : player_view.each() )
  {
    auto *post_death_timer = reg().try_get<Cmp::Player::PostDeathTimer>( entity );
    if ( post_death_timer and *post_death_timer < post_death_timer->get_timeout() ) continue;
    if ( mortality_cmp.state == Cmp::Player::Mortality::State::DEAD )
    {
      if ( auto *extra_life = reg().try_get<Cmp::Player::ExtraLife>( entity ) )
      {
        // respawn at the granting altar only if the extra life is spent in that altar's scene, otherwise at this scene's start
        const auto altar_in_scene = [&]()
        {
          if ( not extra_life->altar_uuid ) return false;
          for ( auto [altar_entt, altar_cmp, altar_uuid_cmp] : reg().view<Cmp::Altar::MultiBlock, Cmp::UUID>().each() )
          {
            if ( altar_uuid_cmp == *extra_life->altar_uuid ) return true;
          }
          return false;
        };
        const auto respawn_position = ( extra_life->respawn_position and altar_in_scene() )
                                          ? *extra_life->respawn_position
                                          : sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( reg() ) );
        on_update_player_position_event( Events::UpdatePlayerPositionEvent( respawn_position ) );
        Factory::Player::remove_player_extra_life( reg() );
        m_sound_bank.get_effect( "player_respawn" ).play();
        Utils::Player::get_stats( reg() ).apply( { Cmp::Stats::Health{ 100 }, {}, {}, {}, {}, {}, {} } );
        mortality_cmp.state = Cmp::Player::Mortality::State::ALIVE;
        mortality_cmp.last_damage_cause = Cmp::Player::Mortality::State::ALIVE;
        reg().remove<Cmp::NoRender>( entity );
        reg().remove<Cmp::Player::PostDeathTimer>( entity );
        reg().remove<Cmp::Player::HazardBalance>( entity );
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

void PlayerSystem::update_heartbeat( sf::Time dt )
{
  auto &hb = Utils::Player::get_heartbeat( reg() );
  const auto &toxidrome = Utils::Player::get_stats( reg() ).toxidrome();

  // Tachycardia/Bradycardia are mutually exclusive, normalise within -1.0 to 1.0
  const float tachycardia = static_cast<float>( toxidrome.at<Cmp::Toxicity::Tachycardia>().value_or( 0 ) );
  const float bradycardia = static_cast<float>( toxidrome.at<Cmp::Toxicity::Bradycardia>().value_or( 0 ) );
  const float severity = ( tachycardia - bradycardia ) / 100.f;

  auto &heartbeat_sfx = m_sound_bank.get_effect( "heartbeat" );
  if ( hb.update( dt, severity ) ) { heartbeat_sfx.play(); }
}

void PlayerSystem::update_repel_npc( sf::Time dt )
{
  auto player_entt = Utils::Player::get_entity( reg() );
  auto *repel_cmp = reg().try_get<Cmp::Player::RepelNpc>( player_entt );
  if ( repel_cmp )
  {
    repel_cmp->m_timer += dt;
    if ( repel_cmp->expired() )
    {
      reg().remove<Cmp::Player::RepelNpc>( player_entt );
      repel_cmp = nullptr;
    }
  }

  // stain the player red while repelling, blinking as the effect runs out
  if ( repel_cmp and repel_cmp->indicator_visible() )
  {
    reg().emplace_or_replace<Cmp::AbsoluteTint>( player_entt, Sys::PersistSystem::get<Cmp::Persist::RepelNpcPlayerTint>( reg() ) );
  }
  else { reg().remove<Cmp::AbsoluteTint>( player_entt ); }
}

void PlayerSystem::update_toxic_immunity( sf::Time dt )
{
  auto player_entt = Utils::Player::get_entity( reg() );
  auto &player_stats = Utils::Player::get_stats( reg() );
  auto *immunity_cmp = reg().try_get<Cmp::Player::ToxicImmunity>( player_entt );
  if ( not immunity_cmp )
  {
    player_stats.set_toxic_immune( false );
    return;
  }
  immunity_cmp->m_timer += dt;
  if ( immunity_cmp->expired() )
  {
    reg().remove<Cmp::Player::ToxicImmunity>( player_entt );
    player_stats.set_toxic_immune( false );
  }
  else { player_stats.set_toxic_immune( true ); }
}

void PlayerSystem::apply_timed_action_side_effects( sf::Time dt )
{
  auto &player_stats = Utils::Player::get_stats( reg() );
  auto player_entt = Utils::Player::get_entity( reg() );
  if ( auto *suppress_timer = reg().try_get<Cmp::Player::MovementSuppressTimer>( player_entt ) ) *suppress_timer += dt;

  // inventory modifiers keep their own per-action timers, so they are updated every frame.
  // NPC proximity modifiers are applied by NpcSystem
  apply_inventory_modifiers( dt );
  apply_plant_proximity_modifiers( dt );

  // Now apply the various stat modifiers if their conditions are met
  static constexpr float kTimedActionSyncClockMax = 1.0;
  m_timed_action_sync_clock += dt;
  if ( m_timed_action_sync_clock.asSeconds() >= kTimedActionSyncClockMax )
  {
    // health effect decrease per toxidrome
    auto apply_health_dmg = [&]( int dmg, Cmp::Player::Mortality::State cause )
    {
      const auto modifier = Cmp::BaseAction( Cmp::Stats::Health{ -dmg }, {}, {}, {}, {}, {} );
      player_stats.apply( modifier );
      reg().emplace_or_replace<Cmp::Player::TookDamage>( player_entt );
      Utils::Player::get_mortality( reg() ).last_damage_cause = cause;
    };
    if ( player_stats.toxidrome().at<Cmp::Toxicity::Hypoxia>() == 100 )
    {
      apply_health_dmg( Sys::PersistSystem::get<Cmp::Persist::HypoxiaHealthDmgPerSec>( reg() ).get_value(),
                        Cmp::Player::Mortality::State::SUFFOCATED );
    }
    if ( player_stats.toxidrome().at<Cmp::Toxicity::Tachycardia>() == 100 )
    {
      apply_health_dmg( Sys::PersistSystem::get<Cmp::Persist::TachycardiaHealthDmgPerSec>( reg() ).get_value(),
                        Cmp::Player::Mortality::State::CARDIACARREST );
    }
    if ( player_stats.toxidrome().at<Cmp::Toxicity::Bradycardia>() == 100 )
    {
      apply_health_dmg( Sys::PersistSystem::get<Cmp::Persist::BradycardiaHealthDmgPerSec>( reg() ).get_value(),
                        Cmp::Player::Mortality::State::CARDIACARREST );
    }
    if ( player_stats.toxidrome().at<Cmp::Toxicity::Venom>() == 100 )
    {
      apply_health_dmg( Sys::PersistSystem::get<Cmp::Persist::VenomHealthDmgPerSec>( reg() ).get_value(), Cmp::Player::Mortality::State::POISONED );
    }
    if ( player_stats.toxidrome().at<Cmp::Toxicity::Vertigo>() == 100 )
    {
      apply_health_dmg( Sys::PersistSystem::get<Cmp::Persist::VertigoHealthDmgPerSec>( reg() ).get_value(),
                        Cmp::Player::Mortality::State::TERRIFIED );
    }
    if ( player_stats.toxidrome().at<Cmp::Toxicity::Hallucinogen>() == 100 )
    {
      apply_health_dmg( Sys::PersistSystem::get<Cmp::Persist::HallucinogenHealthDmgPerSec>( reg() ).get_value(),
                        Cmp::Player::Mortality::State::INSANITY );
    }
    if ( player_stats.fear() == 100 )
    {
      apply_health_dmg( Sys::PersistSystem::get<Cmp::Persist::FearHealthDmgPerSec>( reg() ).get_value(), Cmp::Player::Mortality::State::TERRIFIED );
    }
    if ( player_stats.despair() == 100 )
    {
      apply_health_dmg( Sys::PersistSystem::get<Cmp::Persist::DespairHealthDmgPerSec>( reg() ).get_value(), Cmp::Player::Mortality::State::SUICIDE );
    }
    if ( m_player_is_on_fire )
    {
      apply_health_dmg( Sys::PersistSystem::get<Cmp::Persist::PlayerOnFireHealthDmgPerSec>( reg() ).get_value(),
                        Cmp::Player::Mortality::State::IGNITED );
    }

    m_timed_action_sync_clock = sf::Time::Zero;

    kill_player_if_stats_are_max();
  }
}

void PlayerSystem::update_luck_drop( sf::Time dt )
{
  const float seconds_per_point = Sys::PersistSystem::get<Cmp::Persist::LuckDropRate>( reg() ).get_value();
  if ( seconds_per_point <= 0.f )
  {
    m_luck_drop_timer = sf::Time::Zero;
    return;
  }

  m_luck_drop_timer += dt;
  const sf::Time interval = sf::seconds( seconds_per_point );
  auto &player_stats = Utils::Player::get_stats( reg() );
  while ( m_luck_drop_timer >= interval )
  {
    player_stats.apply( Cmp::BaseAction( {}, {}, {}, {}, Cmp::Stats::Luck{ -1 }, {} ) );
    m_luck_drop_timer -= interval;
  }
}

bool PlayerSystem::is_player_in_light() { return Utils::Collision::is_position_illuminated( reg(), Utils::Player::get_position( reg() ) ); }

void PlayerSystem::apply_inventory_modifiers( sf::Time dt )
{
  // carry effects only apply to the item in hand (the selected slot). We need the inventory component not the types
  const auto slot_entt = Utils::Player::get_inventory_entt( reg() );
  if ( slot_entt == entt::null ) return;
  auto &slot_cmp = reg().get<Cmp::PlayerInventorySlot>( slot_entt );
  {
    auto &[carry_action_def, carry_timer] = slot_cmp.m_item->at<Cmp::CarryAction>();
    if ( carry_action_def.interval() == 0.f ) return;
    carry_timer += dt;
    if ( carry_timer.asSeconds() < carry_action_def.interval() ) return;
    // items without a Cmp::Inventory::StingDamage (e.g. foraged with gloves) are harmless to carry
    auto carry_action = carry_action_def;
    if ( not reg().all_of<Cmp::Inventory::StingDamage>( slot_entt ) ) carry_action.remove_damage();
    if ( carry_action.health() < 0 )
    {
      reg().emplace_or_replace<Cmp::Player::TookDamage>( Utils::Player::get_entity( reg() ) );
      Utils::Player::get_mortality( reg() ).last_damage_cause = Cmp::Player::Mortality::State::SKEWERED;
    }
    Utils::Player::get_stats( reg() ).apply( carry_action );
    get_systems_event_queue().trigger( Events::GrimoireItemUpdateEvent( slot_cmp.m_item->item_type, Cmp::CarryAction::kKey, carry_action ) );
    carry_timer = sf::Time::Zero;
  }
}

void PlayerSystem::apply_plant_proximity_modifiers( sf::Time dt )
{
  std::vector<entt::entity> worn_out_plants;
  for ( auto [plant_entt, plant_mb_cmp, plant_worlditem_cmp] : reg().view<Cmp::PlantMultiBlock, Cmp::WorldItem>().each() )
  {

    auto &[proximity_action, proximity_timer] = plant_worlditem_cmp.at<Cmp::ProximityAction>();

    proximity_timer += dt;
    if ( proximity_timer.asSeconds() < proximity_action.interval() ) continue;

    const Cmp::Position plant_bounds( plant_mb_cmp.position, plant_mb_cmp.size );
    const auto &player_pos = Utils::Player::get_position( reg() );
    auto in_range = [&]( int trigger_range )
    { return Cmp::RectBounds::expanded( plant_mb_cmp, trigger_range ).findIntersection( player_pos ).has_value(); };
    bool triggered = false;

    // spawn NPC now that we are close enough. kill_npc is left to NpcSystem: it is gated on NPCs, not the player
    const auto &fx = proximity_action.side_effects();
    if ( ( fx.spawn_npc or fx.lightning or fx.movement or fx.toxic_immunity ) and ( not fx.spawn_npc or in_range( fx.spawn_npc->trigger_range ) ) )
    {
      trigger_side_effects( proximity_action, plant_bounds, plant_entt );
      triggered = true;
    }

    // drop the reward (e.g. gold) once close enough, independently of the side effects
    if ( const auto &reward = proximity_action.reward();
         reward and ( not reward->loot.empty() or not reward->item.empty() ) and in_range( reward->trigger_range ) )
    {
      drop_proximity_reward( *reward, plant_bounds, plant_entt );
      triggered = true;
    }

    if ( triggered )
    {
      play_action_sfx( proximity_action );
      get_systems_event_queue().trigger( Events::GrimoireItemUpdateEvent( plant_worlditem_cmp.item_type, Cmp::ProximityAction::kKey, proximity_action ) );
      // reset now, otherwise side effects and rewards re-trigger every frame while the player is outside the closer range check below
      proximity_timer = sf::Time::Zero;
    }

    if ( not Utils::Player::is_player_near( reg(), plant_mb_cmp ) ) continue;
    if ( proximity_action.health() < 0 ) reg().emplace_or_replace<Cmp::Player::TookDamage>( Utils::Player::get_entity( reg() ) );

    // snapshot the stats so an "active_only" wear can tell whether the action did anything, e.g. no healing at full health
    const auto &player_stats = Utils::Player::get_stats( reg() );
    auto stat_snapshot = [&]()
    {
      return std::make_tuple( player_stats.health(), player_stats.fear(), player_stats.despair(), player_stats.infamy(), player_stats.luck(),
                              std::unordered_map<entt::id_type, int>( player_stats.toxidrome().begin(), player_stats.toxidrome().end() ) );
    };
    const auto stats_before = stat_snapshot();
    Utils::Player::apply_action_from_world_item<Cmp::ProximityAction>( reg(), plant_entt );
    proximity_timer = sf::Time::Zero;

    // each application wears the plant down over "wear.expiryseconds" of proximity, e.g. st john's wort withers as it heals
    if ( const auto &wear = proximity_action.wear(); wear and wear->expiry_seconds > 0.f and proximity_action.interval() > 0.f )
    {
      if ( wear->active_only and stat_snapshot() == stats_before ) continue;
      get_systems_event_queue().trigger( Events::UpdateDmgEvent( plant_entt, 100.f * proximity_action.interval() / wear->expiry_seconds ) );
      if ( Utils::is_plant_depleted( reg(), plant_entt ) ) worn_out_plants.push_back( plant_entt );
    }
  }
  // a plant worn out by its proximity action withers away, like a fully foraged one. Destroyed after the loop: the
  // event handler modifies the storage being iterated
  for ( auto plant_entt : worn_out_plants )
  {
    get_systems_event_queue().trigger( Events::DestroyPlantEvent( plant_entt ) );
  }
}

void PlayerSystem::drop_proximity_reward( const Cmp::ActionReward &reward, const Cmp::Position &source, entt::entity source_entt )
{
  if ( not reward.loot.empty() )
  {
    get_systems_event_queue().trigger( Events::CreateLootEvent( reward.loot, source, reward.max_instances, source_entt ) );
  }
  else if ( not reward.item.empty() )
  {
    // a proximity reward is dropped nearby by default
    grant_item_reward( reward, source, source_entt );
  }
}

void PlayerSystem::set_player_on_fire()
{
  if ( not Utils::Player::get_stats( reg() ).toxidrome().has<Cmp::Toxicity::Phototoxia>() ) return;

  auto player_pos = Utils::Player::get_position( reg() );
  sf::Vector2f flame_emitter_pos( player_pos.getCenter().x, player_pos.position.y + player_pos.size.y );
  // follow the player sprite's render offset (resting offset plus any flying lift)
  if ( const auto *offset_cmp = reg().try_get<Cmp::AbsoluteRenderOffset>( Utils::Player::get_entity( reg() ) ) )
  {
    flame_emitter_pos += offset_cmp->getOffset();
  }

  // phototoxic skin only burns under a light source, the flames go out in the dark
  auto toxicity = Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Phototoxia>();
  if ( toxicity >= 100 and Utils::Player::is_illuminated( reg() ) )
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

void PlayerSystem::kill_player_if_stats_are_max()
{
  // dont send mortality event twice during death animation
  if ( Utils::Player::get_mortality( reg() ).state == Cmp::Player::Mortality::State::DEAD ) return;
  if ( Utils::Player::get_stats( reg() ).health() > 0 ) return;
  // Now, are we dead and if so, what type of death animation should be triggered?
  if ( Utils::Player::get_stats( reg() ).fear() == 100 )
  {
    on_player_mortality_event( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::TERRIFIED, Utils::Player::get_position( reg() ) ) );
  }
  else if ( Utils::Player::get_stats( reg() ).despair() >= 100 )
  {
    on_player_mortality_event( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::SUICIDE, Utils::Player::get_position( reg() ) ) );
  }
  else if ( Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Hypoxia>() == 100 )
  {
    on_player_mortality_event( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::SUFFOCATED, Utils::Player::get_position( reg() ) ) );
  }
  else if ( Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Tachycardia>() == 100 or
            Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Bradycardia>() == 100 )
  {
    on_player_mortality_event( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::CARDIACARREST, Utils::Player::get_position( reg() ) ) );
  }
  else if ( Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Venom>() == 100 )
  {
    on_player_mortality_event( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::POISONED, Utils::Player::get_position( reg() ) ) );
  }
  else if ( Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Vertigo>() == 100 )
  {
    on_player_mortality_event( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::TERRIFIED, Utils::Player::get_position( reg() ) ) );
  }
  else if ( Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Hallucinogen>() == 100 )
  {
    on_player_mortality_event( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::INSANITY, Utils::Player::get_position( reg() ) ) );
  }
  else if ( Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Phototoxia>() == 100 and m_player_is_on_fire )
  {
    m_player_is_on_fire = false;
    on_player_mortality_event( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::IGNITED, Utils::Player::get_position( reg() ) ) );
  }
  else
  {
    // no maxed stat to blame, so use whatever last damaged the player
    auto cause = Utils::Player::get_mortality( reg() ).last_damage_cause;
    if ( cause == Cmp::Player::Mortality::State::ALIVE )
    {
      SPDLOG_WARN( "Player health is 0 but no damage cause was recorded" );
      cause = Cmp::Player::Mortality::State::EXHAUSTED;
    }
    on_player_mortality_event( Events::PlayerMortalityEvent( cause, Utils::Player::get_position( reg() ) ) );
  }
}

void PlayerSystem::check_burning_particle_collision( std::string_view tag_suffix, int damage, Cmp::Player::Mortality::State death_state )
{
  if ( not Utils::scene_setting<Cmp::SceneSettings::CollisionDetection>( reg() ).enabled ) return;

  // don't spam death events if the player is already dying/dead
  if ( Utils::Player::get_mortality( reg() ).state != Cmp::Player::Mortality::State::ALIVE ) return;

  auto &player_cmp = reg().get<Cmp::Player::Character>( Utils::Player::get_entity( reg() ) );
  if ( not player_cmp.skip_damage_cooldown_once && player_cmp.m_damage_cooldown_timer < player_cmp.damage_cooldown_timeout() ) return;

  const Cmp::Position player_pos_cmp = Utils::Player::get_position( reg() );

  // the collision check also kills the particle that hit the player
  std::optional<sf::Vector2f> emitter_pos;
  for ( auto [ps_entt, ps_owner] : reg().view<Cmp::Particle::SpriteOwner>().each() )
  {
    if ( not ps_owner.sprite->is_active() ) continue;
    if ( not ps_owner.sprite->get_tag().contains( tag_suffix ) ) continue;
    if ( ps_owner.sprite->check_particle_collision( player_pos_cmp ) ) emitter_pos = ps_owner.sprite->get_emitter_position();
  }
  if ( not emitter_pos ) return;

  Cmp::BaseAction burn_action( Cmp::Stats::Health{ -damage }, {}, {}, {}, {}, {} );
  Utils::Player::get_stats( reg() ).apply( burn_action );
  reg().emplace_or_replace<Cmp::Player::TookDamage>( Utils::Player::get_entity( reg() ) );
  player_cmp.skip_damage_cooldown_once = false;
  player_cmp.m_damage_cooldown_timer = sf::Time::Zero;
  m_sound_bank.get_effect( "damage_player" ).play();
  SPDLOG_INFO( "Player (health:{}) hit by {}", Utils::Player::get_stats( reg() ).health(), tag_suffix );

  if ( Utils::Player::get_stats( reg() ).health() <= 0 )
  {
    on_player_mortality_event( Events::PlayerMortalityEvent( death_state, player_pos_cmp ) );
    return;
  }

  try_particle_knockback( *emitter_pos );
}

void PlayerSystem::try_particle_knockback( sf::Vector2f emitter_pos )
{
  const Cmp::Position player_pos_cmp = Utils::Player::get_position( reg() );

  // push the player away from where the particle came from, along whichever axis they are furthest from it
  const sf::Vector2f away = player_pos_cmp.getCenter() - emitter_pos;
  const sf::Vector2f primary_direction = std::abs( away.x ) > std::abs( away.y ) ? sf::Vector2f( away.x < 0.f ? -1.f : 1.f, 0.f )
                                                                                 : sf::Vector2f( 0.f, away.y < 0.f ? -1.f : 1.f );

  // if that cell is blocked fall back to the two sideways directions, but never back towards the source
  std::vector<sf::Vector2f> candidate_directions{ primary_direction };
  for ( auto c : Utils::Cardinal() )
  {
    if ( c.vector() != primary_direction and c.vector() != -primary_direction ) candidate_directions.push_back( c.vector() );
  }

  auto is_blocked = [&]( const Cmp::RectBounds &rect ) -> bool
  {
    for ( auto [nopath_entt, nopath_pos_cmp] : reg().view<Cmp::Player::NoPath, Cmp::Position>().each() )
    {
      if ( rect.findIntersection( nopath_pos_cmp ) ) return true;
    }
    // never knock the player into a lethal hazard cell, e.g. the lava pit itself
    return Utils::Collision::any_lethal_hazard( reg(), rect );
  };

  for ( const auto &direction : candidate_directions )
  {
    auto new_position = Utils::snap_to_grid( player_pos_cmp.position + direction.componentWiseMul( Constants::kGridSizePxF ) );
    if ( is_blocked( Cmp::RectBounds::scaled( new_position, Constants::kGridSizePxF, 1.f ) ) ) continue;

    SPDLOG_INFO( "Particle knockback succeeded: player moved to {},{}", new_position.x, new_position.y );
    on_update_player_position_event( Events::UpdatePlayerPositionEvent( new_position ) );
    return;
  }

  SPDLOG_INFO( "Particle knockback blocked in all directions" );
}

void PlayerSystem::update_note_particles()
{

  auto ps_tag = std::string( Cmp::Particle::NoteParticleSprite::kPlayerNotesTag );
  auto ps_list = Sys::ParticleSystem::find( reg(), ps_tag );

  if ( Utils::Player::is_elderflute_active( reg() ) )
  {
    if ( ps_list.empty() )
    {
      // add missing NoteParticleSprite
      auto uuid_cmp = Cmp::UUID::generate();
      auto player_pos = Utils::Player::get_position( reg() );
      sf::Vector2 note_ps_pos( player_pos.getCenter().x, player_pos.position.y );
      Factory::Particle::add_notes( reg(), ps_tag, uuid_cmp, note_ps_pos, note_ps_pos.y );
    }
    else
    {
      // restart (no-op if already running) and follow the player's feet
      const auto player_pos = Utils::Player::get_position( reg() );
      const sf::Vector2f feet_pos( player_pos.getCenter().x, player_pos.position.y );
      for ( auto [entt, ps] : ps_list )
      {
        ps.get().set_emitter_position( feet_pos );
        ps.get().restart();
      }
    }
  }
  else
  {
    if ( not ps_list.empty() )
    {
      // stop NoteParticleSprites while the elderflute is not playing
      for ( auto [entt, ps] : ps_list )
      {
        ps.get().stop();
      }
    }
  }
}

void PlayerSystem::blink_player()
{
  // damage cooldown blink effect
  for ( auto [player_entt, player_cmp] : reg().view<Cmp::Player::Character>().each() )
  {
    // TeleportSystem owns alpha while a teleport is pending - don't clobber its fade
    if ( reg().all_of<Cmp::PendingTeleport>( player_entt ) ) continue;

    bool is_in_damage_cooldown = player_cmp.m_damage_cooldown_timer < player_cmp.damage_cooldown_timeout();
    bool blink_visible = ( player_cmp.m_damage_cooldown_timer.asMilliseconds() / 100 ) % 2 == 0;

    auto &alpha_cmp = Utils::Player::get_alpha( reg() );
    if ( not is_in_damage_cooldown or player_cmp.skip_damage_cooldown_once or ( is_in_damage_cooldown and blink_visible ) ) { alpha_cmp = 255; }
    else { alpha_cmp = 0; }
  }
}

bool PlayerSystem::is_valid_move( const sf::FloatRect &target_position )
{
  // Shrink the collision query by a sub-pixel margin on each side (centred) so float error at exact grid
  // alignment (e.g. 47.9999 vs 48) doesn't register as an overlap with a neighbouring obstacle.
  constexpr float kCollisionInsetPx = 0.5f;
  const float inset_scale = target_position.size.x > 0.f ? 1.f - ( 2.f * kCollisionInsetPx ) / target_position.size.x : 1.f;
  auto search_bounds = Cmp::RectBounds::scaled( target_position.position, target_position.size, inset_scale );

  // while flying only walls, level boundaries and multiblocks larger than 1x1 block the player
  const bool flying = Utils::Player::is_flying( reg() );
  auto blocks = [&]( entt::entity candidate_entt ) { return not flying or FlyingSystem::blocks_flight( reg(), candidate_entt ); };

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
      if ( search_bounds.findIntersection( *pos_cmp ) and blocks( candidate_entt ) ) return false;
    }
  }
  else
  {
    for ( auto [candidate_entt, candidate_pos] : reg().view<Cmp::Player::NoPath, Cmp::Position>().each() )
    {
      if ( search_bounds.findIntersection( candidate_pos ) and blocks( candidate_entt ) ) return false;
    }
  }

  // flying over hazards (lava etc) - not blocked
  if ( flying ) return true;

  // update_player_position() calls is_valid_move() for both axes every frame, even the axis with
  // zero direction (its target is just the player's current tile), which must never be blocked.
  const auto &player_pos_cmp = Utils::Player::get_position( reg() );
  if ( target_position.position == player_pos_cmp.position ) return true;

  // grass blocks like a wall while Cmp::Persist::GrassSlowFactor is zero (same inset as the obstacle test above)
  const auto current_bounds = Cmp::RectBounds::scaled( player_pos_cmp.position, player_pos_cmp.size, inset_scale );
  if ( Utils::is_blocked_by_grass( reg(), current_bounds.getBounds(), search_bounds.getBounds() ) ) return false;

  // lethal hazard cells block like a wall; update_player_position() starts the balance window when its
  // real forward move is blocked by one (corner-assist/gap-catch probes must not)
  // (the player may step Cmp::Persist::HazardBalanceOverlap onto it, see Utils::Collision::lethal_hazard_probe)
  return not Utils::Collision::any_lethal_hazard( reg(), Utils::Collision::lethal_hazard_probe( reg(), target_position ) );
}

void PlayerSystem::begin_hazard_balance( entt::entity hazard_entt, Utils::Cardinal hazard_cell_dir )
{
  const auto player_entt = Utils::Player::get_entity( reg() );
  if ( reg().all_of<Cmp::Player::HazardBalance>( player_entt ) ) return;

  // the blocked move hasn't stepped more than Cmp::Persist::HazardBalanceOverlap (<= half a cell) onto the hazard yet,
  // so the nearest grid line on the hazard axis is the edge cell's: lean exactly that far out from it over the hazard.
  // The perpendicular axis is left as is.
  const sf::Vector2f player_pos = Utils::Player::get_position( reg() ).position;
  const sf::Vector2f snapped_pos = Utils::snap_to_grid( player_pos );
  const bool horizontal = hazard_cell_dir.vector().x != 0.f;
  const sf::Vector2f edge_pos = horizontal ? sf::Vector2f( snapped_pos.x, player_pos.y ) : sf::Vector2f( player_pos.x, snapped_pos.y );
  const float overlap = std::clamp( Sys::PersistSystem::get<Cmp::Persist::HazardBalanceOverlap>( reg() ).get_value(), 0.f, 0.5f );
  const sf::Vector2f balance_pos = edge_pos + hazard_cell_dir.vector().componentWiseMul( Constants::kGridSizePxF * overlap );

  // recovering steps 1px back along the movement input, so both axes only when moving diagonally
  // (Cmp::Direction components are -1/0/1)
  const sf::Vector2f safe_pos = balance_pos - sf::Vector2f( Utils::Player::get_direction( reg() ) );

  reg().emplace<Cmp::Player::HazardBalance>( player_entt, hazard_entt, hazard_cell_dir, balance_pos, safe_pos );
  on_update_player_position_event( Events::UpdatePlayerPositionEvent( balance_pos ) );
  SPDLOG_INFO( "Player balancing on hazard edge at {},{}", balance_pos.x, balance_pos.y );
}

bool PlayerSystem::update_hazard_balance( sf::Time dt )
{
  const auto player_entt = Utils::Player::get_entity( reg() );
  auto *balance_cmp = reg().try_get<Cmp::Player::HazardBalance>( player_entt );
  if ( not balance_cmp ) return false;

  // the hazard went away (e.g. lava pit removed by a crypt shuffle) - nothing to fall into
  const entt::entity hazard_entt = balance_cmp->hazard;
  const bool is_sinkhole = reg().valid( hazard_entt ) and reg().all_of<Cmp::SinkholeCell>( hazard_entt );
  const bool is_lava = reg().valid( hazard_entt ) and reg().all_of<Cmp::Crypt::RoomLavaPitCell>( hazard_entt );
  if ( not is_sinkhole and not is_lava )
  {
    reg().remove<Cmp::Player::HazardBalance>( player_entt );
    return false;
  }

  // something else moved the player off the edge (fireball/NPC/shockwave knockback) - they're no longer balancing
  if ( Utils::Player::get_position( reg() ).position != balance_cmp->balance_pos )
  {
    reg().remove<Cmp::Player::HazardBalance>( player_entt );
    SPDLOG_INFO( "Player knocked off hazard edge, balance cancelled" );
    return false;
  }

  balance_cmp->elapsed += dt;

  // recovered: pressing away from the hazard steps the player 1px back to safety
  const sf::Vector2f input_dir = Utils::Player::get_direction( reg() );
  if ( input_dir.dot( balance_cmp->hazard_cell_dir.vector() ) < 0.f )
  {
    const sf::Vector2f safe_pos = balance_cmp->safe_pos;
    reg().remove<Cmp::Player::HazardBalance>( player_entt );
    on_update_player_position_event( Events::UpdatePlayerPositionEvent( safe_pos ) );
    SPDLOG_INFO( "Player recovered from hazard edge" );
    return false;
  }

  const float timeout = Sys::PersistSystem::get<Cmp::Persist::HazardBalanceTimeout>( reg() ).get_value();
  if ( balance_cmp->elapsed.asSeconds() < timeout ) return true;

  // timed out: fall into the hazard
  reg().remove<Cmp::Player::HazardBalance>( player_entt );
  const Cmp::Position hazard_pos = reg().get<Cmp::Position>( hazard_entt );
  on_update_player_position_event( Events::UpdatePlayerPositionEvent( hazard_pos.position ) );
  if ( is_sinkhole ) { on_player_mortality_event( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::FALLING, hazard_pos ) ); }
  else
  {
    Utils::Crypt::reset_crypt_shuffle_timer( reg() );
    on_player_mortality_event( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::IGNITED, hazard_pos ) );
  }
  return true;
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
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::DECAYING: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::HAUNTED: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::EXPLODING: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::DROWNING: {
      m_sound_bank.stop_all();
      break;
    }
    case Cmp::Player::Mortality::State::SQUISHED: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::SUICIDE: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::IGNITED: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.flames" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "shrine_lighting" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::SKEWERED: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::SHOCKED: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::TERRIFIED: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::DEAD: {
      m_sound_bank.stop_all();
      break;
    }
    case Cmp::Player::Mortality::State::SHADOWCURSED: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::SUFFOCATED: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::CARDIACARREST: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::POISONED: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::EXHAUSTED:
    case Cmp::Player::Mortality::State::SACRIFICED: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    case Cmp::Player::Mortality::State::INSANITY: {
      m_sound_bank.stop_all();
      const auto &sprite = Sys::SpriteStore::instance().get( "sprite.death.anim.bloodsplat" );
      Factory::Player::create_player_death_anim( reg(), ev.m_death_pos, sprite );
      m_sound_bank.get_effect( "player_blood_splat" ).play();
      common_death_throes();
      break;
    }
    break;
  }
}

} // namespace Game::Sys