
#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteAlpha.hpp>
#include <Components/AnimData.hpp>
#include <Components/DestroyedObstacle.hpp>
#include <Components/Direction.hpp>
#include <Components/FractalCurve.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/LastDirection.hpp>
#include <Components/LootContainer.hpp>
#include <Components/Moveable.hpp>
#include <Components/Npc/Burryman.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Obstacle.hpp>
#include <Components/ObstacleCap.hpp>
#include <Components/Persistent/AxeObstacleDigWear.hpp>
#include <Components/Persistent/AxePotSmashWear.hpp>
#include <Components/Persistent/AxeSkeletonKillWear.hpp>
#include <Components/Persistent/DiggingDamagePerHit.hpp>
#include <Components/Persistent/PickaxeObstacleDigWear.hpp>
#include <Components/Persistent/PickaxePotSmashWear.hpp>
#include <Components/Persistent/ShovelObstacleDigWear.hpp>
#include <Components/Persistent/ShovelPotSmashWear.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/DiggingTimer.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/SelectedPosition.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/UUID.hpp>
#include <Events/DestroyLootContainerEvent.hpp>
#include <Events/DestroyNpcEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <Factory/BombFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SFML/System/Vector2.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/ActionSystem.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Systems/Threats/LightningSystem.hpp>
#include <Systems/Threats/WormholeSystem.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Audio/Sound.hpp>
#include <SFML/System/Time.hpp>
#include <algorithm>
#include <cmath>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <numbers>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace Game::Sys
{

ActionSystem::ActionSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  // The entt::dispatcher is independent of the registry, so it is safe to bind event handlers in
  // the constructor
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&ActionSystem::on_player_action>( this );
  SPDLOG_DEBUG( "ActionSystem initialized" );
}

void ActionSystem::update( [[maybe_unused]] sf::Time dt )
{
  // destroy particle sprite entities once all their particles have expired
  Factory::Particle::delete_expired_particle_sprites( reg(), "graveyard.obstacle.particle.dig" );

  // abort if still in cooldown
  if ( auto *digging_timer = reg().try_get<Cmp::Player::DiggingTimer>( Utils::Player::get_entity( reg() ) ) ) *digging_timer += dt;
  if ( Utils::Player::is_digging_on_cooldown( reg() ) ) { return; }
}

void ActionSystem::on_player_action( const Events::PlayerActionEvent &event )
{
  if ( event.action == Events::PlayerActionEvent::GameActions::DIG )
  {
    // Check for collisions with diggable obstacles
    check_player_dig_obstacle_collision();
    check_player_smash_pot();
  }
  else if ( event.action == Game::Events::PlayerActionEvent::GameActions::ATTACK )
  {
    // axe attack?!
    check_player_axe_npc_kill();
  }
  else if ( event.action == Events::PlayerActionEvent::GameActions::SELECT_POSITION ) { select_moveable_obstacle(); }
  else if ( event.action == Events::PlayerActionEvent::GameActions::DESELECT_POSITION ) { Utils::reset_all_selected_positions( reg() ); }
}

void ActionSystem::check_player_dig_obstacle_collision()
{

  if ( Utils::Player::get_inventory_wear_level( reg() ) <= 0 ) { return; }

  auto [_, inventory_item, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_item != "item.pickaxe" and inventory_item != "item.axe" and inventory_item != "item.shovel" ) return;

  // abort if still in cooldown
  if ( Utils::Player::is_digging_on_cooldown( reg() ) ) { return; }

  // Cooldown has expired: Remove any existing SelectedPosition components from the registry
  Utils::reset_all_selected_positions( reg() );

  // Cap position lookup by UUID, built once so each obstacle below can find its paired cap in O(1)
  // rather than rescanning every cap entity in the level.
  std::unordered_map<Cmp::UUID, Cmp::Position> cap_position_by_uuid;
  for ( auto [cap_entt, cap_cmp, cap_pos_cmp, cap_uuid_cmp] : reg().view<Cmp::ObstacleCap, Cmp::Position, Cmp::UUID>().each() )
  {
    cap_position_by_uuid.emplace( cap_uuid_cmp, cap_pos_cmp );
  }

  auto mouse_position_bounds = Utils::get_mouse_bounds_in_gameview( m_window, RenderSystem::get_world_view() );

  // Iterate through all entities with Position and Obstacle components
  auto position_view = reg().view<Cmp::Position, Cmp::Obstacle, Cmp::AbsoluteAlpha, Cmp::AnimData, Cmp::UUID>( entt::exclude<Cmp::SelectedPosition> );
  for ( auto [obstacle_entt, obstacle_pos_cmp, obstacle_cmp, obstacle_alpha_cmp, obstacle_anim_cmp, obstacle_uuid_cmp] : position_view.each() )
  {
    if ( not obstacle_anim_cmp.m_sprite_type.contains( ".main" ) ) continue;

    // The obstacle's cap is rendered as a separate entity directly above it, and from the
    // front-facing view the player sees obstacle and cap as one continuous shape - so a click on
    // either must select the same obstacle. The cap is matched strictly by its paired UUID (never
    // by position), so a different obstacle stacked above - whose own cap may visually overlap this
    // one - can never be picked up here.
    bool mouse_over_obstacle_or_cap = static_cast<bool>( mouse_position_bounds.findIntersection( obstacle_pos_cmp ) );
    if ( not mouse_over_obstacle_or_cap )
    {
      auto cap_it = cap_position_by_uuid.find( obstacle_uuid_cmp );
      if ( cap_it != cap_position_by_uuid.end() )
      {
        mouse_over_obstacle_or_cap = static_cast<bool>( mouse_position_bounds.findIntersection( cap_it->second ) );
      }
    }

    if ( mouse_over_obstacle_or_cap )
    {
      auto reserved_sm = m_reserved_sm.lock();
      if ( not reserved_sm ) return;

      // check player is near obstacle that was mouse-selected
      if ( not Utils::Player::is_player_near( reg(), obstacle_pos_cmp ) ) continue;

      // check player is facing the obstacle
      if ( not Utils::Player::get_projected_position( reg() ).findIntersection( obstacle_pos_cmp ) ) continue;

      // We are in proximity to an entity that is a candidate for a new SelectedPosition component.
      // Add a new SelectedPosition component to the entity
      reg().emplace_or_replace<Cmp::SelectedPosition>( obstacle_entt, obstacle_pos_cmp.position );
      reg().emplace_or_replace<Cmp::Player::DiggingTimer>( Utils::Player::get_entity( reg() ) );

      // calculate new alpha value and apply to the current obstacle and any obstacle with matching UUID (cap sprite obstacles)
      auto damage_per_hit = Sys::PersistSystem::get<Cmp::Persist::DiggingDamagePerHit>( reg() ).get_value();
      auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
      if ( inventory_type == "item.pickaxe" ) { damage_per_hit = damage_per_hit / 2; }
      else if ( inventory_type == "item.shovel" or inventory_type == "item.axe" ) { damage_per_hit = damage_per_hit / 5; }
      obstacle_cmp.damage += damage_per_hit;

      float dmg = 0.f;
      if ( inventory_type == "item.pickaxe" ) { dmg = Sys::PersistSystem::get<Cmp::Persist::PickaxeObstacleDigWear>( reg() ).get_value(); }
      else if ( inventory_type == "item.axe" ) { dmg = Sys::PersistSystem::get<Cmp::Persist::AxeObstacleDigWear>( reg() ).get_value(); }
      else if ( inventory_type == "item.shovel" ) { dmg = Sys::PersistSystem::get<Cmp::Persist::ShovelObstacleDigWear>( reg() ).get_value(); }
      get_systems_event_queue().trigger( Events::UpdateDmgEvent( Utils::Player::get_inventory_entt( reg() ), dmg ) );

      // Destroy the obstacle?
      if ( obstacle_cmp.damage >= 100 )
      {
        // select the final smash sound
        m_sound_bank.get_effect( "pickaxe_final" ).play();

        // replace the obstacle with a detonated component
        Factory::Obstacle::remove_obstacle( reg(), obstacle_entt, Factory::Obstacle::DeleteExtras::Yes, reserved_sm );
        for ( auto [ob_crack_entt, ob_crack_cmp, ob_crack_uuid] : reg().view<Cmp::ObstacleCrack, Cmp::UUID>().each() )
        {
          if ( ob_crack_uuid == obstacle_uuid_cmp ) reg().destroy( ob_crack_entt );
        }
        // skip if a detonated entity already occupies this position (e.g. overlapping blast patterns)
        bool already_detonated = false;
        Utils::Collision::for_each_intersect<Cmp::DestroyedObstacle>(
            reg(), obstacle_pos_cmp, [&]( entt::entity, Cmp::DestroyedObstacle &, Cmp::Position & ) { already_detonated = true; } );
        if ( not already_detonated ) { Factory::Bomb::add_detonated( reg(), obstacle_entt, obstacle_pos_cmp ); }

        // add the position to the spatial grid so it can be used in pathfinding
        if ( PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = m_npc_navmesh.lock() )
          pathfinding_navmesh->insert( obstacle_entt, obstacle_pos_cmp );
        if ( PathFinding::SpatialHashGridSharedPtr ghost_navmesh = m_ghost_navmesh.lock() ) ghost_navmesh->insert( obstacle_entt, obstacle_pos_cmp );
        if ( PathFinding::SpatialHashGridSharedPtr player_navmesh = m_player_navmesh.lock() )
          player_navmesh->insert( obstacle_entt, obstacle_pos_cmp );

        auto dig_particle_uuid = Cmp::UUID::generate();
        Factory::Particle::add_obstacledig_ps( reg(), "graveyard.obstacle.particle.dig", 50, 2.f, 50.f, 14.f, dig_particle_uuid,
                                               obstacle_pos_cmp.position, obstacle_pos_cmp.y() );
      }
      else
      {
        // select all pickaxe sounds except the final smash sound
        Cmp::RandomInt random_picker( 1, 6 );
        m_sound_bank.get_effect( "pickaxe" + std::to_string( random_picker.gen() ) ).play();

        constexpr int kCracksPerHit = 3;
        for ( auto crack_iter : std::views::iota( 0, kCracksPerHit ) )
        {

          auto dig_particle_uuid = Cmp::UUID::generate();
          Factory::Particle::add_obstacledig_ps( reg(), "graveyard.obstacle.particle.dig", 5, 2.f, 50.f, 7.f, dig_particle_uuid,
                                                 obstacle_pos_cmp.position, obstacle_pos_cmp.y() );

          sf::Vector2f crack_start_pos( obstacle_pos_cmp.getCenter().x, obstacle_pos_cmp.getCenter().y - 12.f ); // offset for obstacle cap
          constexpr float kCrackLengthRatio = 0.4f;
          float crack_length = Constants::kGridSizePxF.x * kCrackLengthRatio;

          // Cracks land on one of the fixed kCrackAngleStepDeg-spaced slots around the circle (picked
          // at random, not in sequence), and never within kCrackAngleStepDeg of a crack already on
          // this obstacle (checked directly via circular distance, not slot indices, since the slot
          // grid itself is rotated per crack_iter below). The grid's own rotation is derived from the
          // obstacle's UUID so it's stable across hits but differs per obstacle instance, with each of
          // the kCracksPerHit cracks generated for this hit additionally offset from one another so
          // they don't all draw from the same rotated grid.
          constexpr float kCrackAngleStepDeg = 70.f;
          constexpr int kCrackSlotCount = static_cast<int>( 360.f / kCrackAngleStepDeg );
          float base_angle_deg = static_cast<float>( std::hash<Cmp::UUID>{}( obstacle_uuid_cmp ) % 360 ) +
                                 ( static_cast<float>( crack_iter ) * ( 360.f / static_cast<float>( kCracksPerHit ) ) );

          auto circular_angle_diff_deg = []( float a, float b )
          {
            float diff = std::fmod( std::abs( a - b ), 360.f );
            return diff > 180.f ? 360.f - diff : diff;
          };

          std::vector<float> used_angles_deg;
          for ( auto [ob_crack_entt, ob_crack_cmp, ob_crack_uuid] : reg().view<Cmp::ObstacleCrack, Cmp::UUID>().each() )
          {
            if ( ob_crack_uuid != obstacle_uuid_cmp ) continue;
            if ( ob_crack_cmp.sequence.size() < 2 or ob_crack_cmp.sequence.front().empty() or ob_crack_cmp.sequence.back().empty() ) continue;

            sf::Vector2f existing_dir = ob_crack_cmp.sequence.back().front().position - ob_crack_cmp.sequence.front().front().position;
            float existing_angle_deg = std::atan2( existing_dir.y, existing_dir.x ) * 180.f / std::numbers::pi_v<float>;
            if ( existing_angle_deg < 0.f ) existing_angle_deg += 360.f;
            used_angles_deg.push_back( existing_angle_deg );
          }

          std::vector<int> free_slots;
          for ( int slot = 0; slot < kCrackSlotCount; ++slot )
          {
            float candidate_deg = std::fmod( base_angle_deg + ( static_cast<float>( slot ) * kCrackAngleStepDeg ), 360.f );
            bool too_close = std::ranges::any_of( used_angles_deg, [&]( float used_deg )
            { return circular_angle_diff_deg( candidate_deg, used_deg ) < kCrackAngleStepDeg; } );
            if ( not too_close ) free_slots.push_back( slot );
          }
          // every slot on this obstacle is already taken - fall back to a plain random pick
          int crack_slot = free_slots.empty() ? Cmp::RandomInt( 0, kCrackSlotCount - 1 ).gen()
                                              : free_slots[Cmp::RandomInt( 0, static_cast<int>( free_slots.size() ) - 1 ).gen()];

          float crack_angle_deg = std::fmod( base_angle_deg + ( static_cast<float>( crack_slot ) * kCrackAngleStepDeg ), 360.f );
          float crack_angle = crack_angle_deg * std::numbers::pi_v<float> / 180.f;
          sf::Vector2f crack_end_pos = crack_start_pos + sf::Vector2f{ std::cos( crack_angle ), std::sin( crack_angle ) } * crack_length;
          Cmp::ObstacleCrack::AngleDeviations crack_angles{ .inner = 1.f, .outer = 1.f };
          Cmp::ObstacleCrack obstacle_crack_cmp( crack_start_pos, crack_end_pos, crack_angles, sf::Time::Zero );
          for ( auto _ : std::views::iota( 0, 3 ) )
          {
            LightningSystem::divide_lightning_segments( obstacle_crack_cmp.sequence, obstacle_crack_cmp.m_deviations, 1 );
          }
          auto entt_main = reg().create();
          reg().emplace_or_replace<Cmp::ObstacleCrack>( entt_main, obstacle_crack_cmp );
          reg().emplace_or_replace<Cmp::UUID>( entt_main, obstacle_uuid_cmp );
        }
      }
    }
  }
}

void ActionSystem::check_player_smash_pot()
{

  auto [_, inventory_slot_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_slot_type != "item.pickaxe" and inventory_slot_type != "item.axe" and inventory_slot_type != "item.shovel" ) { return; }

  if ( Utils::Player::get_inventory_wear_level( reg() ) <= 0 ) { return; }

  // abort if still in cooldown
  if ( Utils::Player::is_digging_on_cooldown( reg() ) ) { return; }

  auto mouse_position_bounds = Utils::get_mouse_bounds_in_gameview( m_window, RenderSystem::get_world_view() );
  // Cmp::LootContainer is an empty tag type, so the view yields only the entity and its Cmp::Position
  auto loot_container_view = reg().view<Cmp::LootContainer, Cmp::Position>();

  entt::entity smashed_loot_entity = entt::null;

  for ( auto [loot_entity, loot_container_pos] : loot_container_view.each() )
  {
    if ( mouse_position_bounds.findIntersection( loot_container_pos ) )
    {
      SPDLOG_DEBUG( "Found lootable entity at position: [{}, {}]!", loot_container_pos.position.x, loot_container_pos.position.y );

      // check player is near obstacle that was mouse-selected
      if ( not Utils::Player::is_player_near( reg(), loot_container_pos ) ) continue;

      // check player is facing the obstacle
      if ( not Utils::Player::get_projected_position( reg() ).findIntersection( loot_container_pos ) ) continue;

      reg().emplace_or_replace<Cmp::Player::DiggingTimer>( Utils::Player::get_entity( reg() ) );

      float dmg = 0.f;
      if ( inventory_slot_type == "item.pickaxe" ) { dmg = Sys::PersistSystem::get<Cmp::Persist::PickaxePotSmashWear>( reg() ).get_value(); }
      else if ( inventory_slot_type == "item.axe" ) { dmg = Sys::PersistSystem::get<Cmp::Persist::AxePotSmashWear>( reg() ).get_value(); }
      else if ( inventory_slot_type == "item.shovel" ) { dmg = Sys::PersistSystem::get<Cmp::Persist::ShovelPotSmashWear>( reg() ).get_value(); }
      get_systems_event_queue().trigger( Events::UpdateDmgEvent( Utils::Player::get_inventory_entt( reg() ), dmg ) );

      // wait until we're outside of the view loop to break the container: LootSystem spawns loot with a Cmp::Position
      smashed_loot_entity = loot_entity;
      break;
    }
  }

  if ( smashed_loot_entity != entt::null ) get_systems_event_queue().trigger( Events::DestroyLootContainerEvent( smashed_loot_entity ) );
}

void ActionSystem::check_player_axe_npc_kill()
{
  PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = m_npc_navmesh.lock();
  if ( not pathfinding_navmesh ) return;

  auto [_, inventory_slot_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_slot_type != "item.axe" ) { return; }

  if ( Utils::Player::get_inventory_wear_level( reg() ) <= 0 ) { return; }

  // Remove any existing SelectedPosition from NPCs only — this runs every frame the attack button is
  // held with no cooldown of its own, so clearing the whole registry's SelectedPosition here would
  // also wipe unrelated selections (e.g. the obstacle currently being dug) set by other systems
  auto selected_position_view = reg().view<Cmp::SelectedPosition, Cmp::Npc::NPC>();
  for ( auto [existing_sel_entity, sel_cmp, npc_cmp] : selected_position_view.each() )
  {
    reg().remove<Cmp::SelectedPosition>( existing_sel_entity );
  }

  // Iterate through all entities with Position and Obstacle components
  auto position_view = reg().view<Cmp::Position, Cmp::Npc::NPC, Cmp::AnimData>( entt::exclude<Cmp::SelectedPosition> );
  SPDLOG_DEBUG( "position_view size: {}", position_view.size_hint() );
  for ( auto [npc_entity, npc_pos_cmp, npc_cmp, anim_cmp] : position_view.each() )
  {
    // only skeletons and the Burryman can be axed
    const bool is_skeleton = anim_cmp.m_sprite_type.contains( "sprite.skeleton" );
    const bool is_burryman = reg().all_of<Cmp::Npc::Burryman>( npc_entity );

    if ( not is_skeleton and not is_burryman ) continue;
    auto mouse_position_bounds = Utils::get_mouse_bounds_in_gameview( m_window, RenderSystem::get_world_view() );
    if ( mouse_position_bounds.findIntersection( npc_pos_cmp ) )
    {
      SPDLOG_DEBUG( "Found NPC entity at position: [{}, {}]!", npc_pos_cmp.position.x, npc_pos_cmp.position.y );

      // TODO: check player is facing the obstacle
      // Check player proximity to the entity
      bool player_nearby = false;
      for ( auto [pc_entt, pc_cmp, pc_pos_cmp] : reg().view<Cmp::Player::Character, const Cmp::Position>().each() )
      {
        auto player_hitbox = Cmp::RectBounds::scaled( pc_pos_cmp.position, Constants::kGridSizePxF, 1.5f );
        if ( player_hitbox.findIntersection( npc_pos_cmp ) )
        {
          player_nearby = true;
          break;
        }
      }

      // skip this iteration of the loop if player too far away
      if ( not player_nearby ) { continue; }

      // We are in proximity to an entity that is a candidate for a new SelectedPosition component.
      // Add a new SelectedPosition component to the entity
      reg().emplace_or_replace<Cmp::SelectedPosition>( npc_entity, npc_pos_cmp.position );

      float dmg = Sys::PersistSystem::get<Cmp::Persist::AxeSkeletonKillWear>( reg() ).get_value();
      get_systems_event_queue().trigger( Events::UpdateDmgEvent( Utils::Player::get_inventory_entt( reg() ), dmg ) );

      // select the final smash sound, played by the DestroyNpcEvent handler
      m_sound_bank.get_effect( "axe_whip" ).play();
      std::optional<std::string> death_sfx;
      if ( is_skeleton )
      {
        death_sfx = "rattling_bones";
        auto skelebones_particle_uuid = Cmp::UUID::generate();
        Factory::Particle::add_skelebones_ps( reg(), "graveyard.skele.particle.bones", 50, 2.f, 50.f, 14.f, skelebones_particle_uuid,
                                              npc_pos_cmp.getCenter(), npc_pos_cmp.position.y );
      }
      else if ( is_burryman )
      {
        death_sfx = "chopping_final";
        auto burryman_twigs_particle_uuid = Cmp::UUID::generate();
        Factory::Particle::add_planttwigs_ps( reg(), "graveyard.plant.particle.twigs", 10, 2.f, 50.f, 14.f, burryman_twigs_particle_uuid,
                                              npc_pos_cmp.getCenter(), npc_pos_cmp.position.y );
      }

      SPDLOG_DEBUG( "Killed NPC at position ({}, {})!", npc_pos_cmp.position.x, npc_pos_cmp.position.y );
      // now destroy the NPC
      get_systems_event_queue().trigger( Events::DestroyNpcEvent( npc_entity, std::nullopt, death_sfx, Events::DestroyNpcEvent::Cause::AXE ) );
    }
  }
}

void ActionSystem::select_moveable_obstacle()
{
  // project one grid position in the direction that the player is currently facing to find the obstacle selection
  auto selected_position = Utils::Player::get_projected_position( reg() );

  auto reserved_sm = m_reserved_sm.lock();
  auto position_view = reg().view<Cmp::Position, Cmp::Obstacle, Cmp::Moveable>( entt::exclude<Cmp::SelectedPosition> );
  for ( auto [obst_entity, obst_pos_cmp, obst_cmp, move_cmp] : position_view.each() )
  {
    if ( reserved_sm && not reserved_sm->at( obst_pos_cmp ).empty() ) continue;
    if ( selected_position.findIntersection( obst_pos_cmp ) )
    {
      SPDLOG_INFO( "Found moveable entity at position: [{}, {}]!", obst_pos_cmp.position.x, obst_pos_cmp.position.y );
      reg().emplace_or_replace<Cmp::SelectedPosition>( obst_entity, obst_pos_cmp.position );
    }
  }
}

} // namespace Game::Sys