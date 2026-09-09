#include <Audio/SoundBank.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/AnimData.hpp>
#include <Components/Crypt/BuildingMultiBlock.hpp>
#include <Components/Crypt/BuildingSegment.hpp>
#include <Components/Crypt/Chest.hpp>
#include <Components/Crypt/Entrance.hpp>
#include <Components/Crypt/Lever.hpp>
#include <Components/Crypt/ObjectiveMultiBlock.hpp>
#include <Components/Crypt/ObjectiveSegment.hpp>
#include <Components/Crypt/PassageBlock.hpp>
#include <Components/Crypt/PassageDoor.hpp>
#include <Components/Crypt/PassageSpikeTrap.hpp>
#include <Components/Crypt/RoomClosed.hpp>
#include <Components/Crypt/RoomEnd.hpp>
#include <Components/Crypt/RoomLavaPit.hpp>
#include <Components/Crypt/RoomLavaPitCell.hpp>
#include <Components/Crypt/RoomLavaPitCellEffect.hpp>
#include <Components/Crypt/RoomOpen.hpp>
#include <Components/Crypt/RoomStart.hpp>
#include <Components/Direction.hpp>
#include <Components/Exit.hpp>
#include <Components/FootStepAlpha.hpp>
#include <Components/FootStepTimer.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Persistent/CryptNpcSpawnCount.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/CadaverCount.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/Stats/SpawnAction.hpp>

#include <Components/VoidPosition.hpp>
#include <Components/Wall.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/CryptRoomEvent.hpp>
#include <Events/PassageEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Factory/CryptFactory.hpp>
#include <Factory/LootFactory.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Factory/SpriteFactory.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <SceneControl/Scenes/CryptScene.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/CryptSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Crypt.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Audio/Sound.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entity/fwd.hpp>
#include <stdexcept>

namespace Game::Sys
{

void CryptSystem::setup()
{
  if ( Utils::Player::get_mortality( m_reg ).state != Cmp::Player::Mortality::State::DEAD )
  {
    shuffle_rooms_passages();
    reset_maze();
  }
}

void CryptSystem::update( sf::Time dt )
{

  if ( m_maze_unlocked ) { Factory::Crypt::destroy_crypt_shuffle_timer( reg() ); }
  else { Utils::Crypt::update_crypt_shuffle_timer( reg(), dt ); }

  check_exit_collision();

  if ( not m_maze_unlocked and Utils::Crypt::is_crypt_shuffle_timer_expired( reg() ) )
  {
    //
    shuffle_rooms_passages();
  }

  // check collisions with lava pit/spike traps
  if ( Utils::scene_setting<Cmp::SceneSettings::CollisionDetection>( reg() ).enabled )
  {
    check_lava_pit_collision();
    check_spike_trap_collision();
  }

  check_lever_activation();
  check_lava_pit_activation_by_proximity();
  do_lava_pit_animation();
  check_spike_trap_activation_by_proximity();
}

void CryptSystem::shuffle_rooms_passages()
{
  auto uuid_map = build_non_obstacle_uuid_map();

  auto selected_rooms = Utils::Rnd::get_n_rand_components<Cmp::Crypt::RoomClosed>(
      reg(), 4, {}, Utils::Rnd::ExcludePack<Cmp::Crypt::RoomStart, Cmp::Crypt::RoomEnd>{}, 0 );

  auto player_pos_cmp = Utils::Player::get_position( reg() );
  // reset rooms/passages
  remove_lava_pit_open_rooms( player_pos_cmp );
  remove_lever_open_rooms( player_pos_cmp );
  remove_chest_open_rooms( player_pos_cmp );
  close_open_rooms( player_pos_cmp );
  fill_closed_rooms( uuid_map );
  get_systems_event_queue().trigger( Events::PassageEvent( Events::PassageEvent::Type::REMOVE_PASSAGES ) );

  // open new rooms/passages
  open_selected_rooms( selected_rooms );
  empty_open_rooms( uuid_map );
  create_room_borders( uuid_map );
  add_lava_pit_open_rooms( player_pos_cmp );

  // try to open passages for the occupied room: only do start room if player is currently there
  auto [start_room_entt, start_room_cmp] = get_crypt_room_start();
  if ( Utils::Player::get_position( reg() ).findIntersection( start_room_cmp ) )
  {
    get_systems_event_queue().trigger( Events::PassageEvent( Events::PassageEvent::Type::CONNECT_START_TO_OPENROOMS, start_room_entt ) );
  }
  else { get_systems_event_queue().trigger( Events::PassageEvent( Events::PassageEvent::Type::CONNECT_OCCUPIED_TO_OPENROOMS ) ); }

  add_chest_to_open_rooms( player_pos_cmp );
  get_systems_event_queue().trigger( Events::PassageEvent( Events::PassageEvent::Type::OPEN_PASSAGES ) );
  add_lever_to_open_rooms();

  m_sound_bank.get_effect( "crypt_room_shuffle" ).play();
  Utils::Crypt::reset_crypt_shuffle_timer( reg() );
}

void CryptSystem::unlock_objective_passage()
{
  auto uuid_map = build_non_obstacle_uuid_map();

  auto player_pos_cmp = Utils::Player::get_position( reg() );

  // reset rooms/passages
  remove_lava_pit_open_rooms( player_pos_cmp );
  remove_lever_open_rooms( player_pos_cmp );
  remove_chest_open_rooms( player_pos_cmp );
  close_open_rooms( player_pos_cmp );
  remove_all_levers();
  fill_closed_rooms( uuid_map );
  get_systems_event_queue().trigger( Events::PassageEvent( Events::PassageEvent::Type::REMOVE_PASSAGES ) );
  empty_open_rooms( uuid_map );
  create_room_borders( uuid_map );

  // open new rooms/passages
  SPDLOG_DEBUG( "~~~~~~~~~~~ OPENING FINAL PASSAGE ~~~~~~~~~~~~~~~" );
  get_systems_event_queue().trigger( Events::PassageEvent( Events::PassageEvent::Type::CONNECT_OCCUPIED_TO_ENDROOM, get_crypt_room_end().first ) );
  get_systems_event_queue().trigger( Events::PassageEvent( Events::PassageEvent::Type::OPEN_PASSAGES ) );
  m_sound_bank.get_effect( "crypt_room_shuffle" ).play();
}

void CryptSystem::unlock_exit_passage()
{
  auto uuid_map = build_non_obstacle_uuid_map();

  auto player_pos_cmp = Utils::Player::get_position( reg() );

  // if we unlocked the maze by picking up the cadaver, then cancel the timer
  m_maze_unlocked = true;

  close_open_rooms( player_pos_cmp );
  remove_all_chests();
  remove_all_levers();
  remove_lava_pit_open_rooms( player_pos_cmp );
  open_all_rooms();
  empty_open_rooms( uuid_map );
  create_room_borders( uuid_map );

  // make sure player can reach exit
  get_systems_event_queue().trigger( Events::PassageEvent( Events::PassageEvent::Type::CONNECT_START_TO_OPENROOMS, get_crypt_room_start().first ) );
  get_systems_event_queue().trigger( Events::PassageEvent( Events::PassageEvent::Type::CONNECT_ALL_ROOMS ) );

  spawn_npc_in_open_rooms();
}

void CryptSystem::on_player_action( Events::PlayerActionEvent &event )
{
  if ( Utils::Player::get_mortality( reg() ).state == Cmp::Player::Mortality::State::DEAD ) return;

  if ( event.action == Events::PlayerActionEvent::GameActions::ACTIVATE ) check_objective_activation( event.action );
  if ( event.action == Events::PlayerActionEvent::GameActions::ACTIVATE ) check_chest_activation( event.action );
}

void CryptSystem::on_room_event( Events::CryptRoomEvent &event )
{
  if ( Utils::Player::get_mortality( reg() ).state == Cmp::Player::Mortality::State::DEAD ) return;

  if ( event.type == Events::CryptRoomEvent::Type::SHUFFLE_PASSAGES ) { shuffle_rooms_passages(); }
  else if ( event.type == Events::CryptRoomEvent::Type::FINAL_PASSAGE ) { unlock_objective_passage(); }
  else if ( event.type == Events::CryptRoomEvent::Type::EXIT_ALL_PASSAGES ) { unlock_exit_passage(); }
}

void CryptSystem::update_exit_zorder()
{
  auto player_pos = Utils::Player::get_position( reg() );

  for ( auto [mb_entt, mb_cmp, mb_z_cmp] : reg().view<Cmp::Crypt::BuildingMultiBlock, Cmp::ZOrderValue>().each() )
  {
    if ( not player_pos.findIntersection( mb_cmp ) ) continue;
    auto segment_view = reg().view<Cmp::Crypt::BuildingSegment, Cmp::Position, Cmp::ZOrderValue>();
    for ( auto [segment_entt, segment_cmp, segment_pos_cmp, segment_z_cmp] : segment_view.each() )
    {
      if ( not player_pos.findIntersection( segment_pos_cmp ) ) continue;
      mb_z_cmp.setZOrder( segment_z_cmp.getZOrder() );
      SPDLOG_DEBUG( "Updated zorder to {}", segment_z_cmp.getZOrder() );
    }
  }
}

void CryptSystem::check_entrance_collision()
{
  auto player_pos = Utils::Player::get_position( reg() );
  auto door_view = reg().view<Cmp::Crypt::Entrance, Cmp::Position>();
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [door_entity, door_cmp, door_pos_cmp] : door_view.each() )
  {
    // optimize: skip if not visible
    if ( not Utils::is_visible_in_view( view_bounds, door_pos_cmp ) ) continue;

    // shrink entrance bounds slightly for better UX
    auto decreased_entrance_bounds = Cmp::RectBounds::scaled( door_pos_cmp.position, door_pos_cmp.size, 0.1f, Cmp::RectBounds::ScaleAxis::XY );

    if ( not player_pos.findIntersection( decreased_entrance_bounds.getBounds() ) ) continue;
    m_scenemanager_event_dispatcher.enqueue<Events::SceneManagerEvent>( Events::SceneManagerEvent::Type::ENTER_CRYPT );

    auto [_, inventory_slot_type, _] = Utils::Player::get_inventory( reg() );

    Factory::Player::remove_player_last_graveyard_pos( reg() );
    Cmp::Position last_known_pos(
        {
            door_pos_cmp.position.x,
            door_pos_cmp.position.y + Constants::kGridSizePxF.y,
        },
        Constants::kGridSizePxF );
    SPDLOG_INFO( "Last known graveyard position {}, {}", last_known_pos.position.x, last_known_pos.position.y );
    Factory::Player::add_player_last_graveyard_pos( reg(), last_known_pos );
    break;
  }
}

void CryptSystem::check_exit_collision()
{
  auto player_pos = Utils::Player::get_position( reg() );
  auto door_view = reg().view<Cmp::Exit, Cmp::Position>();
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [door_entity, door_cmp, door_pos_cmp] : door_view.each() )
  {
    // optimize: skip if not visible
    if ( !Utils::is_visible_in_view( view_bounds, door_pos_cmp ) ) continue;

    auto decreased_entrance_bounds = Cmp::RectBounds::scaled( door_pos_cmp.position, door_pos_cmp.size, 0.1f,
                                                              Cmp::RectBounds::ScaleAxis::XY ); // shrink entrance bounds slightly for better UX

    if ( not player_pos.findIntersection( decreased_entrance_bounds.getBounds() ) ) continue;

    m_scenemanager_event_dispatcher.enqueue<Events::SceneManagerEvent>( Events::SceneManagerEvent::Type::EXIT_CRYPT );
  }
}

void CryptSystem::unlock_crypt_door()
{

  auto player_pos_cmp = Utils::Player::get_position( reg() );
  auto cryptdoor_view = reg().view<Cmp::Crypt::Entrance, Cmp::Position>();

  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type != "item.cryptkey" ) return;

  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );
  for ( auto [door_entity, cryptdoor_cmp, door_pos_cmp] : cryptdoor_view.each() )
  {
    // optimize: skip if not visible
    if ( !Utils::is_visible_in_view( view_bounds, door_pos_cmp ) ) continue;

    // Player can't intersect with a closed crypt door so expand their hitbox to facilitate collision detection
    auto player_hitbox = Cmp::RectBounds::scaled( player_pos_cmp, 5.f );
    if ( not player_hitbox.findIntersection( door_pos_cmp ) ) continue;

    // Crypt door is already opened
    if ( cryptdoor_cmp.is_open() )
    {
      // Set the z-order value
      auto crypt_entt = find_intersecting_multiblock( door_pos_cmp );
      if ( crypt_entt != entt::null )
      {
        auto &crypt_cmp = reg().get<Cmp::Crypt::BuildingMultiBlock>( crypt_entt );
        reg().emplace_or_replace<Cmp::ZOrderValue>( crypt_entt, crypt_cmp.position.y - 16.f );
        if ( reg().any_of<Cmp::Player::NoPath>( crypt_entt ) )
        {
          SPDLOG_DEBUG( "CryptDoor is open" );
          reg().remove<Cmp::Player::NoPath>( crypt_entt );
        }
      }
      continue;
    }
    {
      // Set the z-order value
      auto crypt_entt = find_intersecting_multiblock( door_pos_cmp );
      if ( crypt_entt != entt::null )
      {
        auto &crypt_cmp = reg().get<Cmp::Crypt::BuildingMultiBlock>( crypt_entt );
        reg().emplace_or_replace<Cmp::ZOrderValue>( crypt_entt, crypt_cmp.position.y + 16.f );
      }
    }

    // unlock the crypt door
    SPDLOG_DEBUG( "Player unlocked a crypt door at ({}, {})", door_pos_cmp.position.x, door_pos_cmp.position.y );
    Factory::Player::destroy_inventory( reg(), "item.cryptkey" );

    // player_key_count->decrement_count( 1 );
    m_sound_bank.get_effect( "crypt_open" ).play();
    cryptdoor_cmp.set_is_open( true );
    reg().remove<Cmp::Player::NoPath>( door_entity );

    // make doorway non-solid and lower z-order so player walks over it
    reg().emplace_or_replace<Cmp::Crypt::BuildingSegment>( door_entity, Cmp::Crypt::BuildingSegment( false ) );

    // find the crypt multi-block this door belongs to and update the sprite
    auto crypt_view = reg().view<Cmp::Crypt::BuildingMultiBlock, Cmp::AnimData>();
    for ( auto [crypt_entity, crypt_cmp, anim_cmp] : crypt_view.each() )
    {
      if ( not door_pos_cmp.findIntersection( crypt_cmp ) ) continue;
      anim_cmp.m_sprite_type = "sprite.graveyard.crypt.opened";

      SPDLOG_DEBUG( "Updated crypt multi-block sprite to open state at ({}, {})", crypt_cmp.position.x, crypt_cmp.position.y );
      break;
    }
  }
}

void CryptSystem::check_objective_activation( Events::PlayerActionEvent::GameActions action )
{
  if ( action != Events::PlayerActionEvent::GameActions::ACTIVATE ) return;

  auto player_pos_cmp = Utils::Player::get_position( reg() );
  auto player_hitbox = Cmp::RectBounds::scaled( player_pos_cmp.position, Constants::kGridSizePxF, 1.5f );

  for ( auto [objective_entity, objective_cmp] : reg().view<Cmp::Crypt::ObjectiveMultiBlock>().each() )
  {
    // did we already get the cadaver from this objective?
    if ( objective_cmp.get_activation_count() >= objective_cmp.get_activation_threshold() ) continue;

    if ( player_hitbox.findIntersection( objective_cmp ) )
    {
      sf::FloatRect expanded_search( sf::Vector2f{ objective_cmp.position.x, objective_cmp.position.y + objective_cmp.size.y },
                                     sf::Vector2f{ objective_cmp.size.x, Constants::kGridSizePxF.y * 2.f } );
      auto obst_entity = Factory::Loot::create_loot_drop(
          reg(), Cmp::AnimData( Cmp::AnimData::Config{ .sprite_type = "sprite.crypt.loot.cadaver", .enabled = true } ), expanded_search,
          Factory::IncludePack<>{}, Factory::ExcludePack<Cmp::Player::Character, Cmp::Crypt::ObjectiveSegment>{} );
      if ( obst_entity != entt::null )
      {
        m_sound_bank.get_effect( "drop_loot" ).play();
        objective_cmp.increment_activation_count();
        SPDLOG_DEBUG( "Player activated crypt objective." );

        const auto &ms = m_sprite_factory.get_spritesheet_by_type( "sprite.crypt.objective.opened" );
        // clang-format off
          reg().emplace_or_replace<Cmp::AnimData>( objective_entity, Cmp::AnimData::Config{ 
                .sprite_type = ms.get_sprite_type(),
                .enabled = true
          });
        // clang-format on
      }
    }
  }
}

void CryptSystem::check_lever_activation()
{
  if ( m_maze_unlocked ) return;

  auto player_view = reg().view<Cmp::Player::Character, Cmp::Position>();
  auto lever_view = reg().view<Cmp::Crypt::Lever, Cmp::Position>();

  for ( auto [pc_entity, player_cmp, player_pos_cmp] : player_view.each() )
  {
    auto player_hitbox = Cmp::RectBounds::scaled( player_pos_cmp.position, Constants::kGridSizePxF, 0.5f );
    for ( auto [lever_entt, lever_cmp, lever_pos_cmp] : lever_view.each() )
    {
      // prevent player from spamming lever twice
      if ( lever_cmp.isEnabled() ) { continue; }
      if ( not player_hitbox.findIntersection( lever_pos_cmp ) ) { continue; }
      lever_cmp.setEnabled( true );
      m_enabled_levers++;

      // clang-format off
      reg().emplace_or_replace<Cmp::AnimData>( lever_entt, Cmp::AnimData::Config{ 
            .sprite_type = "sprite.crypt.switch", 
            .frame_index_offset = 1,
            .enabled = true
      });
      // clang-format on

      m_sound_bank.get_effect( "crypt_lever_open" ).play();
      SPDLOG_DEBUG( "Lever enabled at {},{} - Count:{}", lever_pos_cmp.position.x, lever_pos_cmp.position.y, m_enabled_levers );

      // check if we have activated enough levers to access the maze objective
      if ( m_enabled_levers > 2 and not m_maze_unlocked )
      {
        m_maze_unlocked = true;

        unlock_objective_passage();
        Utils::Crypt::reset_crypt_shuffle_timer( reg() );
      }
      else { shuffle_rooms_passages(); }
    }
  }
}

void CryptSystem::check_chest_activation( Events::PlayerActionEvent::GameActions action )
{
  if ( action != Events::PlayerActionEvent::GameActions::ACTIVATE ) return;

  auto player_view = reg().view<Cmp::Player::Character, Cmp::Position>();
  auto chest_view = reg().view<Cmp::Crypt::Chest, Cmp::Position, Cmp::AnimData>();

  for ( auto [pc_entity, pc_cmp, pc_pos_cmp] : player_view.each() )
  {
    auto player_hitbox = Cmp::RectBounds::scaled( pc_pos_cmp.position, Constants::kGridSizePxF, 1.5f );
    for ( auto [chest_entt, chest_cmp, chest_pos_cmp, chest_anim_cmp] : chest_view.each() )
    {
      // prevent player from spamming chest twice
      if ( chest_cmp.open ) continue;
      if ( not player_hitbox.findIntersection( chest_pos_cmp ) ) continue;

      chest_cmp.open = true;
      chest_anim_cmp.m_enabled = true;
      m_sound_bank.get_effect( "crypt_chest_open" ).play();

      // clang-format off
      auto reserved_sm = m_reserved_sm.lock();
      auto loot_entt = Factory::Loot::create_loot_drop(
        reg(),
        Cmp::AnimData( Cmp::AnimData::Config{ .sprite_type = "sprite.crypt.loot.gold", .enabled = true}),
        Cmp::RectBounds::scaled( chest_pos_cmp, 3.f ).getBounds(),
        Factory::IncludePack<>{},
        Factory::ExcludePack<Cmp::Player::Character, Cmp::Crypt::Chest, Cmp::Crypt::RoomLavaPitCell, Cmp::Crypt::PassageBlock, Cmp::Wall, Cmp::Obstacle>{} ,
        Factory::ExcludePack<Cmp::Player::Character, Cmp::Crypt::Chest, Cmp::Crypt::RoomLavaPitCell, Cmp::Crypt::PassageBlock, Cmp::Wall, Cmp::Obstacle>{},
        64.f, reserved_sm.get());
      // clang-format on

      if ( loot_entt != entt::null ) m_sound_bank.get_effect( "drop_loot" ).play();
    }
  }
}

void CryptSystem::create_room_borders( const Factory::Obstacle::UUIDEntityMap &uuid_map )
{
  auto reserved_sm = m_reserved_sm.lock();
  auto add_borders_for_room = [&]<typename Component>( Component &room_cmp, RoomWallType room_wall_type )
  {
    for ( auto &[pos_entt, pos_cmp] : room_cmp.m_border_position_list )
    {
      if ( not reg().valid( pos_entt ) ) continue;
      auto *anim_data = reg().try_get<Cmp::AnimData>( pos_entt );
      if ( anim_data and anim_data->m_sprite_type.contains( ".main" ) )
      {
        Factory::Obstacle::remove_obstacle( reg(), pos_entt, Factory::Obstacle::DeleteExtras::Yes, uuid_map, reserved_sm );
        decorate_interior_wall( pos_entt, pos_cmp, room_wall_type );
      }
      if ( PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = m_npc_navmesh.lock() ) pathfinding_navmesh->remove( pos_entt, pos_cmp );
    }
  };

  for ( auto [closed_room_entt, closed_room_cmp] : reg().view<Cmp::Crypt::RoomClosed>().each() )
    add_borders_for_room( closed_room_cmp, RoomWallType::INTERIOR );

  for ( auto [open_room_entt, open_room_cmp] : reg().view<Cmp::Crypt::RoomOpen>().each() )
    add_borders_for_room( open_room_cmp, RoomWallType::BORDER );
}

void CryptSystem::gen_crypt_initial_interior()
{
  SPDLOG_DEBUG( "Generating crypt interior obstacles." );

  // Snapshot the pre-existing reservations (walls/spawn/exit/markers from build_scene_from_data)
  // rather than checking the live grid: decorate_interior_wall() below reserves a *different*
  // position for each wall's cap sprite (one tile above the wall), and checking the live grid
  // here would let one wall's cap phantom-reserve an unrelated, still-undecided tile - including
  // genuinely open room floor - leaving it permanently marked reserved despite never being walled.
  auto reserved_sm = m_reserved_sm.lock();
  PathFinding::SpatialHashGrid pre_existing_reserved;
  if ( reserved_sm ) pre_existing_reserved = *reserved_sm;

  auto position_view = reg().view<Cmp::Position>( entt::exclude<Cmp::Player::Character> );
  for ( auto [pos_entt, pos_cmp] : position_view.each() )
  {
    if ( not pre_existing_reserved.at( pos_cmp ).empty() ) continue;
    bool add_interior_wall = true;
    for ( auto [start_room_entity, start_room_cmp] : reg().view<Cmp::Crypt::RoomStart>().each() )
    {
      if ( pos_cmp.findIntersection( start_room_cmp ) ) add_interior_wall = false;
    }
    for ( auto [end_room_entity, end_room_cmp] : reg().view<Cmp::Crypt::RoomEnd>().each() )
    {
      if ( pos_cmp.findIntersection( end_room_cmp ) ) add_interior_wall = false;
    }
    for ( auto [open_room_entity, open_room_cmp] : reg().view<Cmp::Crypt::RoomOpen>().each() )
    {
      if ( pos_cmp.findIntersection( open_room_cmp ) ) add_interior_wall = false;
    }

    if ( add_interior_wall ) { decorate_interior_wall( pos_entt, pos_cmp, RoomWallType::INTERIOR ); }
  }
}

void CryptSystem::create_initial_closed_rooms( sf::Vector2u map_grid_size )
{
  const auto &grid_square_size = Constants::kGridSizePxF;
  const int min_room_width = 3;
  const int min_room_height = 3;
  const int max_room_width = 8;
  const int max_room_height = 8;
  const int max_distance_between_rooms = 2;
  const int max_attempts = 5000;

  auto reserved_sm = m_reserved_sm.lock();

  int room_count = 0;
  int current_attempt = 0;
  while ( room_count < 20 )
  {

    int room_width = Cmp::RandomInt{ min_room_width, max_room_width }.gen();
    int room_height = Cmp::RandomInt{ min_room_height, max_room_height }.gen();
    auto [_, pos] = Utils::Rnd::get_random_position( reg(), {}, Utils::Rnd::ExcludePack<>{}, 0 );
    sf::Vector2f new_room_size( static_cast<float>( room_width ) * grid_square_size.x, static_cast<float>( room_height ) * grid_square_size.y );
    Cmp::Crypt::RoomClosed new_room( pos.position, new_room_size );
    SPDLOG_DEBUG( "Generated new room at ({}, {}) size ({}, {})", new_room.position.x, new_room.position.y, new_room.size.x, new_room.size.y );

    auto is_min_distance_ok = [&]( const Cmp::Crypt::RoomClosed &existing_room, const Cmp::Crypt::RoomClosed &new_room ) -> bool
    {
      float distance_x = std::max( 0.f, std::max( existing_room.position.x - ( new_room.position.x + new_room.size.x ),
                                                  new_room.position.x - ( existing_room.position.x + existing_room.size.x ) ) );
      float distance_y = std::max( 0.f, std::max( existing_room.position.y - ( new_room.position.y + new_room.size.y ),
                                                  new_room.position.y - ( existing_room.position.y + existing_room.size.y ) ) );
      float distance = std::sqrt( ( distance_x * distance_x ) + ( distance_y * distance_y ) );
      return distance >= static_cast<float>( max_distance_between_rooms ) * grid_square_size.x;
    };

    auto check_collision = [&]( const auto &existing_object ) -> bool
    { return new_room.findIntersection( existing_object ) || not is_min_distance_ok( existing_object, new_room ); };

    bool overlaps_existing = false;

    // make sure new_room area does not fall outside map_grid_size
    if ( not Utils::is_in_bounds( new_room.position, new_room.size, map_grid_size ) ) { overlaps_existing = true; }

    // make sure the room's origin doesn't land on an already-reserved position (walls, markers, etc.)
    if ( not overlaps_existing && reserved_sm && not reserved_sm->at( pos ).empty() ) { overlaps_existing = true; }

    // check for intersection with existing rooms
    if ( not overlaps_existing )
    {
      auto room_view = reg().view<Cmp::Crypt::RoomClosed>();
      for ( auto [existing_entity, existing_room] : room_view.each() )
      {
        if ( check_collision( existing_room ) )
        {
          overlaps_existing = true;
          break;
        }
      }
    }

    // check for intersection with walls
    if ( not overlaps_existing )
    {
      auto wall_view = reg().view<Cmp::Wall, Cmp::Position>();
      for ( auto [wall_entity, wall_cmp, wall_pos_cmp] : wall_view.each() )
      {
        if ( check_collision( wall_pos_cmp ) )
        {
          overlaps_existing = true;
          break;
        }
      }
    }

    // check for intersection with start room
    if ( not overlaps_existing )
    {
      auto start_room_view = reg().view<Cmp::Crypt::RoomStart>();
      for ( auto [start_room_entity, start_room_cmp] : start_room_view.each() )
      {
        if ( check_collision( start_room_cmp ) )
        {
          overlaps_existing = true;
          break;
        }
      }
    }

    // check for intersection with end room
    if ( not overlaps_existing )
    {
      auto end_room_view = reg().view<Cmp::Crypt::RoomEnd>();
      for ( auto [end_room_entity, end_room_cmp] : end_room_view.each() )
      {
        if ( check_collision( end_room_cmp ) )
        {
          overlaps_existing = true;
          break;
        }
      }
    }

    // now check the result
    if ( not overlaps_existing )
    {
      auto new_room_entt = reg().create();

      float room_left = new_room.position.x;
      float room_right = new_room.position.x + new_room.size.x;
      float room_top = new_room.position.y;
      float room_bottom = new_room.position.y + new_room.size.y;

      sf::FloatRect border_area(
          sf::Vector2f{ room_left - Constants::kGridSizePxF.x, room_top - Constants::kGridSizePxF.y },
          sf::Vector2f{ new_room.size.x + ( Constants::kGridSizePxF.x * 2.f ), new_room.size.y + ( Constants::kGridSizePxF.y * 2.f ) } );

      for ( auto [pos_entt, pos_cmp] : reg().view<Cmp::Position>().each() )
      {
        if ( reserved_sm && not reserved_sm->at( pos_cmp ).empty() ) continue;
        if ( reg().any_of<Cmp::FootStepTimer, Cmp::FootStepAlpha, Cmp::Direction>( pos_entt ) ) continue;
        if ( reg().any_of<Cmp::Wall, Cmp::Exit>( pos_entt ) ) continue;

        // interior positions
        if ( pos_cmp.findIntersection( new_room ) ) { new_room.m_position_list.emplace_back( pos_entt, pos_cmp ); }
        // border positions (1-tile ring outside the room, excluding interior)
        else if ( border_area.contains( pos_cmp.position ) )
        {
          bool is_border = false;
          if ( pos_cmp.position.x == room_left - Constants::kGridSizePxF.x && pos_cmp.position.y >= room_top - Constants::kGridSizePxF.y &&
               pos_cmp.position.y <= room_bottom )
          {
            is_border = true;
          }
          else if ( pos_cmp.position.x == room_right && pos_cmp.position.y >= room_top - Constants::kGridSizePxF.y &&
                    pos_cmp.position.y <= room_bottom )
          {
            is_border = true;
          }
          else if ( pos_cmp.position.y == room_top - Constants::kGridSizePxF.y && pos_cmp.position.x >= room_left - Constants::kGridSizePxF.x &&
                    pos_cmp.position.x <= room_right )
          {
            is_border = true;
          }
          else if ( pos_cmp.position.y == room_bottom && pos_cmp.position.x >= room_left - Constants::kGridSizePxF.x &&
                    pos_cmp.position.x <= room_right )
          {
            is_border = true;
          }
          if ( is_border ) { new_room.m_border_position_list.emplace_back( pos_entt, pos_cmp ); }
        }
      }

      reg().emplace<Cmp::Crypt::RoomClosed>( new_room_entt, std::move( new_room ) );
      room_count++;
    }
    else
    {
      SPDLOG_DEBUG( "New room overlaps existing room, discarded. ({}/{})", current_attempt, max_attempts );
      current_attempt++;
    }
    if ( current_attempt >= max_attempts )
    {
      SPDLOG_WARN( "Max attempts reached when generating crypt rooms, stopping." );
      return;
    }
  }

  SPDLOG_DEBUG( "Total rooms: {}", room_count );
  // Currently no special logic needed; placeholder for future use
}

void CryptSystem::cache_all_room_connections()
{
  SPDLOG_DEBUG( "CryptSystem::cache_all_room_connections()" );
  get_systems_event_queue().trigger( Events::PassageEvent( Events::PassageEvent::Type::CACHE_ALL_ROOM_CONNECTIONS ) );
}

void CryptSystem::create_end_room( sf::Vector2u map_grid_size )
{
  auto map_grid_sizef = sf::Vector2f( static_cast<float>( map_grid_size.x ) * Constants::kGridSizePxF.x,
                                      static_cast<float>( map_grid_size.y ) * Constants::kGridSizePxF.y );
  auto kGridSizePxF = Constants::kGridSizePxF;
  // target position for the objective: always center top of the map
  const auto &ms = m_sprite_factory.get_spritesheet_by_type( "sprite.crypt.objective.closed" );

  float centered_x = ( map_grid_sizef.x / 2.f ) - ( ms.get_sprite_size().x / 2.f );
  Cmp::Position objective_position( { centered_x, kGridSizePxF.y * 2.f }, ms.get_sprite_size() );

  auto end_room_entity = reg().create();
  reg().emplace_or_replace<Cmp::Crypt::RoomEnd>( end_room_entity,
                                                 sf::Vector2f{ objective_position.position.x - kGridSizePxF.x, objective_position.position.y },
                                                 sf::Vector2f{ objective_position.size.x, objective_position.size.y + ( kGridSizePxF.y * 2.f ) } );
}

/// PRIVATE FUNCTIONS

Factory::Obstacle::UUIDEntityMap CryptSystem::build_non_obstacle_uuid_map()
{
  Factory::Obstacle::UUIDEntityMap uuid_map;
  for ( auto [e, uuid] : reg().view<Cmp::UUID>().each() )
    if ( not reg().all_of<Cmp::Obstacle>( e ) ) uuid_map[uuid] = e;
  return uuid_map;
}

entt::entity CryptSystem::find_intersecting_multiblock( const Cmp::Position &pos_cmp )
{
  for ( auto [crypt_entt, crypt_cmp] : reg().view<Cmp::Crypt::BuildingMultiBlock>().each() )
  {
    if ( pos_cmp.findIntersection( crypt_cmp ) ) return crypt_entt;
  }
  return entt::null;
}

void CryptSystem::decorate_interior_wall( entt::entity main_entt, Cmp::Position &main_pos_cmp, RoomWallType room_wall_type )
{
  const Sprites::SpriteSheet &ss_main = m_sprite_factory.get_spritesheet_by_type( "sprite.crypt.wall.int.main" );
  const Sprites::SpriteSheet &ss_cap = m_sprite_factory.get_spritesheet_by_type( "sprite.crypt.wall.int.cap" );
  auto uuid = Cmp::UUID::generate();

  int tile_idx = static_cast<int>( room_wall_type );
  Factory::Obstacle::add_obstacle( reg(), main_entt );
  Factory::Obstacle::decorate_obstacle( reg(), main_entt, main_pos_cmp, ss_main, tile_idx, main_pos_cmp.y() + ss_main.get_zorder( tile_idx ) );
  reg().emplace_or_replace<Cmp::UUID>( main_entt, uuid );

  // We place the room borders on top of the regular interior wall block so to prevent z-fighting,
  // we add the json zorder to the y-axis position as the total zorder
  auto cap_entt = reg().create();
  Cmp::Position cap_position( { main_pos_cmp.x(), main_pos_cmp.y() - main_pos_cmp.size.y }, main_pos_cmp.size );
  reg().emplace_or_replace<Cmp::Position>( cap_entt, cap_position );
  Factory::Obstacle::decorate_obstacle( reg(), cap_entt, cap_position, ss_cap, tile_idx, main_pos_cmp.y() + ss_cap.get_zorder( tile_idx ), false );
  if ( auto reserved_sm = m_reserved_sm.lock() ) reserved_sm->insert( cap_entt, cap_position );
  reg().emplace_or_replace<Cmp::UUID>( cap_entt, uuid );
  Factory::Obstacle::add_obstacle_cap( reg(), cap_entt );
}

void CryptSystem::close_open_rooms( const Cmp::Position &player_pos_cmp )
{
  // gather all the open rooms into a list
  for ( auto [open_room_entt, open_room_cmp] : reg().view<Cmp::Crypt::RoomOpen>().each() )
  {
    if ( open_room_cmp.findIntersection( player_pos_cmp ) ) continue;

    // transfer the position list to the new open room before destroying the closed room
    Cmp::Crypt::RoomClosed new_closed_room( open_room_cmp.position, open_room_cmp.size );
    new_closed_room.m_position_list = open_room_cmp.m_position_list;
    new_closed_room.m_border_position_list = open_room_cmp.m_border_position_list;

    reg().emplace<Cmp::Crypt::RoomClosed>( open_room_entt, std::move( new_closed_room ) );
    reg().remove<Cmp::Crypt::RoomOpen>( open_room_entt );
  }
}

void CryptSystem::fill_closed_rooms( const Factory::Obstacle::UUIDEntityMap &uuid_map )
{
  PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = m_npc_navmesh.lock();
  if ( not pathfinding_navmesh ) throw std::runtime_error( "CryptSystem::fill_closed_rooms() - unable to lock pathfinding navmesh" );
  auto reserved_sm = m_reserved_sm.lock();

  for ( auto [closed_room_entt, closed_room_cmp] : reg().view<Cmp::Crypt::RoomClosed>().each() )
  {
    for ( auto [pos_entt, pos_cmp] : closed_room_cmp.m_position_list )
    {
      if ( reg().all_of<Cmp::Obstacle>( pos_entt ) ) continue;
      if ( reg().any_of<Cmp::FootStepTimer, Cmp::FootStepAlpha, Cmp::Direction>( pos_entt ) ) continue;

      Factory::Obstacle::remove_obstacle( reg(), pos_entt, Factory::Obstacle::DeleteExtras::Yes, uuid_map, reserved_sm );
      decorate_interior_wall( pos_entt, pos_cmp, RoomWallType::INTERIOR );
      pathfinding_navmesh->remove( pos_entt, pos_cmp );
      if ( auto player_navmesh = m_player_navmesh.lock() ) { player_navmesh->insert( pos_entt, pos_cmp ); }
    }
  }

  for ( auto [closed_room_entt, closed_room_cmp] : reg().view<Cmp::Crypt::RoomClosed>().each() )
  {
    for ( auto [pos_entt, pos_cmp] : closed_room_cmp.m_border_position_list )
    {
      // skip position outside closed room area or if position already has existing obstacle
      // if ( not closed_room_cmp.findIntersection( pos_cmp ) ) continue;
      if ( reg().all_of<Cmp::Obstacle>( pos_entt ) ) continue;
      if ( reg().any_of<Cmp::FootStepTimer, Cmp::FootStepAlpha, Cmp::Direction>( pos_entt ) ) continue;

      Factory::Obstacle::remove_obstacle( reg(), pos_entt, Factory::Obstacle::DeleteExtras::Yes, uuid_map, reserved_sm );
      decorate_interior_wall( pos_entt, pos_cmp, RoomWallType::BORDER );
      pathfinding_navmesh->remove( pos_entt, pos_cmp );
      if ( auto player_navmesh = m_player_navmesh.lock() ) { player_navmesh->insert( pos_entt, pos_cmp ); }
    }
  }
}

void CryptSystem::open_selected_rooms( const std::set<entt::entity> &selected_rooms )
{
  for ( auto [closed_room_entt, closed_room_cmp] : reg().view<Cmp::Crypt::RoomClosed>().each() )
  {
    if ( selected_rooms.find( closed_room_entt ) == selected_rooms.end() ) continue;

    // transfer the position list to the new open room before destroying the closed room
    Cmp::Crypt::RoomOpen new_open_room( closed_room_cmp.position, closed_room_cmp.size );
    new_open_room.m_position_list = closed_room_cmp.m_position_list;
    new_open_room.m_border_position_list = closed_room_cmp.m_border_position_list;

    reg().emplace<Cmp::Crypt::RoomOpen>( closed_room_entt, std::move( new_open_room ) );
    reg().remove<Cmp::Crypt::RoomClosed>( closed_room_entt );
  }
}

void CryptSystem::open_all_rooms()
{
  std::set<entt::entity> all_rooms;
  for ( auto [closed_room_entt, closed_room_cmp] : reg().view<Cmp::Crypt::RoomClosed>().each() )
  {
    all_rooms.insert( closed_room_entt );
  }
  open_selected_rooms( all_rooms );
}

void CryptSystem::empty_open_rooms( const Factory::Obstacle::UUIDEntityMap &uuid_map )
{
  auto reserved_sm = m_reserved_sm.lock();

  for ( auto [open_room_entt, open_room_cmp] : reg().view<Cmp::Crypt::RoomOpen>().each() )
  {
    for ( auto [pos_entt, pos_cmp] : open_room_cmp.m_position_list )
    {
      // skip position outside open room area or if position doesn't have existing obstacle
      if ( not open_room_cmp.findIntersection( pos_cmp ) ) continue;
      if ( not reg().all_of<Cmp::Obstacle>( pos_entt ) ) continue;

      Factory::Obstacle::remove_obstacle( reg(), pos_entt, Factory::Obstacle::DeleteExtras::Yes, uuid_map, reserved_sm );
      if ( PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = m_npc_navmesh.lock() ) { pathfinding_navmesh->insert( pos_entt, pos_cmp ); }
      if ( auto player_navmesh = m_player_navmesh.lock() ) { player_navmesh->remove( pos_entt, pos_cmp ); }
    }

    for ( auto [pos_entt, pos_cmp] : open_room_cmp.m_border_position_list )
    {
      if ( not reg().all_of<Cmp::Obstacle>( pos_entt ) ) continue;
      Factory::Obstacle::remove_obstacle( reg(), pos_entt, Factory::Obstacle::DeleteExtras::Yes, uuid_map, reserved_sm );
      decorate_interior_wall( pos_entt, pos_cmp, RoomWallType::BORDER );
    }
  }
}

void CryptSystem::add_lava_pit_open_rooms( const Cmp::Position &player_pos_cmp )
{
  PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = m_npc_navmesh.lock();
  if ( not pathfinding_navmesh ) { throw std::runtime_error( "CryptSystem::add_lava_pit_open_rooms - Could not access SpatialHashGridSharedPtr" ); }
  auto open_room_view = reg().view<Cmp::Crypt::RoomOpen>();
  for ( auto [open_room_entt, open_room_cmp] : open_room_view.each() )
  {
    if ( open_room_cmp.findIntersection( player_pos_cmp ) ) continue; // skip occupied rooms
    Factory::Crypt::create_crypt_lava_pit( reg(), open_room_cmp, pathfinding_navmesh );
  }
}

void CryptSystem::do_lava_pit_animation()
{
  // remove any pre-existing lava anim entities
  auto lava_anim_view = reg().view<Cmp::Crypt::RoomLavaPitCellEffect, Cmp::AnimData>();
  for ( auto [lava_anim_entt, lava_cell_anim_cmp, lava_anim_cmp] : lava_anim_view.each() )
  {
    // only delete the entity if it has finished its animation sequence
    if ( lava_anim_cmp.m_enabled ) continue;
    if ( reg().valid( lava_anim_entt ) ) reg().destroy( lava_anim_entt );
  }

  if ( m_lava_effect_cooldown_timer.getElapsedTime() > m_lava_effect_cooldown_threshold )
  {
    // add N new lava anim entities
    auto found_lava_cell_entts = Utils::Rnd::get_n_rand_components<Cmp::Crypt::RoomLavaPitCell>( reg(), 1, {}, {} );

    for ( auto lava_cell_entt : found_lava_cell_entts )
    {
      // disabled CryptRoomLavaPitCells have no zorder
      auto *lava_zorder_cmp = reg().try_get<Cmp::ZOrderValue>( lava_cell_entt );
      if ( not lava_zorder_cmp ) continue;

      auto *lava_cell_cmp = reg().try_get<Cmp::Crypt::RoomLavaPitCell>( lava_cell_entt );
      if ( not lava_cell_cmp ) continue;

      auto lava_anim_entt = reg().create();

      reg().emplace_or_replace<Cmp::Crypt::RoomLavaPitCellEffect>( lava_anim_entt );
      reg().emplace_or_replace<Cmp::Position>( lava_anim_entt, lava_cell_cmp->position, lava_cell_cmp->size );
      // clang-format off
      reg().emplace_or_replace<Cmp::AnimData>( lava_anim_entt, Cmp::AnimData::Config{ 
            .sprite_type = "sprite.crypt.anim.lava", 
            .framerate = 0.2,
            .enabled = true,
            .anim_type = Cmp::AnimType::ONESHOTRESET
      });
      // clang-format on
      reg().emplace_or_replace<Cmp::ZOrderValue>( lava_anim_entt, lava_cell_cmp->position.y + 64.f );
    }
    m_lava_effect_cooldown_timer.restart();
  }
}

void CryptSystem::remove_lava_pit_open_rooms( const Cmp::Position &player_pos_cmp )
{
  auto lava_pit_view = reg().view<Cmp::Crypt::RoomLavaPit>();
  for ( auto [open_room_entt, open_room_cmp] : reg().view<Cmp::Crypt::RoomOpen>().each() )
  {
    if ( open_room_cmp.findIntersection( player_pos_cmp ) ) continue; // skip occupied rooms
    for ( auto [lava_pit_entt, lava_pit_cmp] : lava_pit_view.each() )
    {
      if ( not open_room_cmp.findIntersection( lava_pit_cmp ) ) continue;
      if ( PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = m_npc_navmesh.lock() )
      {
        Factory::Crypt::destroy_crypt_lava_pit( reg(), lava_pit_entt, pathfinding_navmesh );
      }
    }
  }
}

void CryptSystem::check_lava_pit_collision()
{
  if ( Utils::Player::get_mortality( reg() ).state == Cmp::Player::Mortality::State::DEAD ) return;

  // full size hitbox - Cmp::Hazard::CollisionResist now guards against accidental entry
  auto player_hitbox = Cmp::RectBounds::scaled( Utils::Player::get_position( reg() ).position, Utils::Player::get_position( reg() ).size, 1.f );
  for ( auto [lava_cell_entt, lava_cell_cmp] : reg().view<Cmp::Crypt::RoomLavaPitCell>().each() )
  {
    if ( not player_hitbox.findIntersection( lava_cell_cmp ) ) continue;
    Utils::Crypt::reset_crypt_shuffle_timer( reg() );

    get_systems_event_queue().enqueue( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::IGNITED, lava_cell_cmp ) );
  }
}

void CryptSystem::check_lava_pit_activation_by_proximity()
{
  auto player_pos_cmp = Utils::Player::get_position( reg() );
  auto player_hitbox_enable = Cmp::RectBounds::scaled( player_pos_cmp.position, player_pos_cmp.size, 2.f );
  auto player_hitbox_disable = Cmp::RectBounds::scaled( player_pos_cmp.position, player_pos_cmp.size, 5.f );
  int num_lava_pits_intersect = 0;
  for ( auto [lava_pit_entt, lava_pit_cmp] : reg().view<Cmp::Crypt::RoomLavaPit>().each() )
  {
    if ( player_hitbox_enable.findIntersection( lava_pit_cmp ) ) num_lava_pits_intersect++;
    for ( auto [lava_cell_entt, lava_cell_cmp] : reg().view<Cmp::Crypt::RoomLavaPitCell>().each() )
    {

      if ( not lava_cell_cmp.findIntersection( lava_pit_cmp ) ) continue;
      if ( player_hitbox_enable.findIntersection( lava_pit_cmp ) )
      {
        // enable rendering of this cell by adding a zorder component
        reg().emplace_or_replace<Cmp::ZOrderValue>( lava_cell_entt, lava_cell_cmp.position.y );
      }
      else if ( not player_hitbox_disable.findIntersection( lava_pit_cmp ) )
      {
        // disable the rendering of this cell by removing its zorder component
        reg().remove<Cmp::ZOrderValue>( lava_cell_entt );
      }
    }
  }

  // finally play/stop sound if any/all lava pits activated/deactivated
  if ( num_lava_pits_intersect > 0 )
  {
    auto sfx_status = m_sound_bank.get_effect( "bubbling_lava" ).getStatus();
    if ( sfx_status != sf::Sound::Status::Playing ) m_sound_bank.get_effect( "bubbling_lava" ).play();
  }
  else
  {
    //
    m_sound_bank.get_effect( "bubbling_lava" ).stop();
  }
}

void CryptSystem::check_spike_trap_collision()
{
  if ( Utils::Player::get_mortality( reg() ).state == Cmp::Player::Mortality::State::DEAD ) return;

  Cmp::Position player_pos = Utils::Player::get_position( reg() );
  auto player_hitbox = Cmp::RectBounds::scaled( player_pos.position, player_pos.size, 0.5f );
  for ( auto [spike_trap_entt, spike_trap_cmp, spike_trap_anim_cmp] : reg().view<Cmp::Crypt::PassageSpikeTrap, Cmp::AnimData>().each() )
  {
    if ( not spike_trap_anim_cmp.m_enabled ) continue;

    Cmp::Position spike_trap_hitbox( spike_trap_cmp, Constants::kGridSizePxF );
    if ( not player_hitbox.findIntersection( spike_trap_hitbox ) ) continue;
    Utils::Crypt::reset_crypt_shuffle_timer( reg() );
    get_systems_event_queue().enqueue( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::SKEWERED, spike_trap_hitbox ) );
  }
}

void CryptSystem::check_spike_trap_activation_by_proximity()
{
  auto player_pos_cmp = Utils::Player::get_position( reg() );
  auto player_hitbox_enable = Cmp::RectBounds::scaled( player_pos_cmp.position, player_pos_cmp.size, 2.f );
  auto player_hitbox_disable = Cmp::RectBounds::scaled( player_pos_cmp.position, player_pos_cmp.size, 5.f );
  for ( auto [spike_trap_entt, spike_trap_cmp, spike_trap_anim_cmp] : reg().view<Cmp::Crypt::PassageSpikeTrap, Cmp::AnimData>().each() )
  {
    auto spike_trap_hitbox = sf::FloatRect( spike_trap_cmp, Constants::kGridSizePxF );
    if ( player_hitbox_enable.findIntersection( spike_trap_hitbox ) and
         spike_trap_cmp.m_cooldown_timer.getElapsedTime() > spike_trap_cmp.m_cooldown_threshold )
    {
      // re-activate the threat
      SPDLOG_DEBUG( "Reactivating spike: {}", static_cast<int>( spike_trap_entt ) );
      spike_trap_anim_cmp.m_enabled = true;
      spike_trap_cmp.m_cooldown_timer.reset();

      auto sfx_status = m_sound_bank.get_effect( "spike_trap" ).getStatus();
      if ( sfx_status != sf::Sound::Status::Playing ) { m_sound_bank.get_effect( "spike_trap" ).play(); }
    }
    else if ( not player_hitbox_disable.findIntersection( spike_trap_hitbox ) )
    {
      // de-activate the threat
      spike_trap_anim_cmp.m_enabled = false;
      spike_trap_anim_cmp.m_current_frame = spike_trap_anim_cmp.m_base_frame;
      spike_trap_cmp.m_cooldown_timer.restart();
    }
  }
}

std::vector<entt::entity> CryptSystem::get_available_room_positions()
{
  std::vector<entt::entity> internal_room_entts;

  for ( auto [open_room_entt, open_room_cmp] : reg().view<Cmp::Crypt::RoomOpen>().each() )
  {
    if ( Utils::Player::get_position( reg() ).findIntersection( open_room_cmp ) ) continue;

    // use pre-built position list instead of iterating all positions
    for ( auto &[pos_entt, pos_cmp] : open_room_cmp.m_position_list )
    {
      if ( not reg().valid( pos_entt ) ) continue;

      bool intersects_lava = false;
      for ( auto [lava_pit_entt, lava_pit_cmp] : reg().view<Cmp::Crypt::RoomLavaPit>().each() )
      {
        if ( pos_cmp.findIntersection( lava_pit_cmp ) )
        {
          intersects_lava = true;
          break;
        }
      }
      if ( intersects_lava ) continue;

      bool intersects_lever = false;
      for ( auto [lever_entt, lever_cmp, lever_pos_cmp] : reg().view<Cmp::Crypt::Lever, Cmp::Position>().each() )
      {
        if ( pos_cmp.findIntersection( lever_pos_cmp ) )
        {
          intersects_lever = true;
          break;
        }
      }
      if ( intersects_lever ) continue;

      bool intersects_chest = false;
      for ( auto [chest_entt, chest_cmp, chest_pos_cmp] : reg().view<Cmp::Crypt::Chest, Cmp::Position>().each() )
      {
        if ( pos_cmp.findIntersection( chest_pos_cmp ) )
        {
          intersects_chest = true;
          break;
        }
      }
      if ( intersects_chest ) continue;

      bool intersects_passageblock = false;
      for ( auto [pblock_entt, pblock_cmp] : reg().view<Cmp::Crypt::PassageBlock>().each() )
      {
        auto expanded_hitbox = Cmp::RectBounds::scaled( pos_cmp.position, pos_cmp.size, 4.f );
        if ( expanded_hitbox.findIntersection( sf::FloatRect( pblock_cmp, Constants::kGridSizePxF ) ) )
        {
          intersects_passageblock = true;
          break;
        }
      }
      if ( intersects_passageblock ) continue;

      internal_room_entts.push_back( pos_entt );
    }
  }

  return internal_room_entts;
}

void CryptSystem::add_chest_to_open_rooms( const Cmp::Position &player_pos_cmp )
{
  auto is_invalid_position = [&]( const Cmp::Crypt::RoomOpen &open_room_cmp, const sf::Vector2f &selected_pos ) -> bool
  {
    auto open_room_width = open_room_cmp.position.x + open_room_cmp.size.x;

    // skip bottom edge and all corner border positons
    if ( selected_pos.y >= open_room_cmp.position.y + ( open_room_cmp.size.y - Constants::kGridSizePxF.y ) ) return true;
    if ( selected_pos.x >= open_room_width and selected_pos.y <= open_room_cmp.position.y ) return true;
    if ( selected_pos.x <= open_room_cmp.position.x and selected_pos.y <= open_room_cmp.position.y ) return true;
    return false;
  };

  for ( auto [open_room_entt, open_room_cmp] : reg().view<Cmp::Crypt::RoomOpen>().each() )
  {
    if ( open_room_cmp.findIntersection( player_pos_cmp ) ) continue;
    Cmp::RandomInt room_border_picker( 0, static_cast<int>( open_room_cmp.m_border_position_list.size() ) - 1 );

    size_t iterations = 0;
    bool invalid_pos_found = true;
    while ( invalid_pos_found and iterations < open_room_cmp.m_border_position_list.size() )
    {
      auto [selected_entt, selected_pos] = open_room_cmp.m_border_position_list[room_border_picker.gen()];

      invalid_pos_found = is_invalid_position( open_room_cmp, selected_pos.position );
      iterations++;
      if ( invalid_pos_found ) { continue; }

      float zorder = selected_pos.y() + m_sprite_factory.get_spritesheet_by_type( "sprite.crypt.chest" ).get_zorder( 0 );

      Factory::Obstacle::remove_obstacle( reg(), selected_entt, Factory::Obstacle::DeleteExtras::Yes, m_reserved_sm.lock() );
      auto chest_entt = Factory::Crypt::create_crypt_chest( reg(), selected_pos.position, "sprite.crypt.chest", 0, zorder );
      if ( auto player_navmesh = m_player_navmesh.lock() ) { player_navmesh->insert( chest_entt, selected_pos ); }
      SPDLOG_DEBUG( "Added chest to position: {},{}", selected_pos.position.x, selected_pos.position.y );
    }
  }
}

void CryptSystem::add_lever_to_open_rooms()
{
  auto internal_room_entts = get_available_room_positions();
  Sprites::SpriteMetaType lever_sprite_type = "sprite.crypt.switch";
  unsigned int disabled_lever_sprite_idx = 0;
  float zorder = m_sprite_factory.get_sprite_size_by_type( lever_sprite_type ).y;

  // add one lever to one room picked from the pool of candidates room positions
  Cmp::RandomInt room_position_picker( 0, static_cast<int>( internal_room_entts.size() ) - 1 );
  auto selected_entt = internal_room_entts[room_position_picker.gen()];
  auto room_pos = reg().get<Cmp::Position>( selected_entt );
  Factory::Crypt::create_crypt_lever( reg(), room_pos.position, lever_sprite_type, disabled_lever_sprite_idx, zorder );
  SPDLOG_DEBUG( "Added lever to position: {},{}", room_pos.position.x, room_pos.position.y );
}

void CryptSystem::remove_lever_open_rooms( const Cmp::Position &player_pos_cmp )
{
  for ( auto [lever_entt, lever_cmp, lever_pos_cmp] : reg().view<Cmp::Crypt::Lever, Cmp::Position>().each() )
  {
    for ( auto [open_room_entt, open_room_cmp] : reg().view<Cmp::Crypt::RoomOpen>().each() )
    {
      if ( not open_room_cmp.findIntersection( lever_pos_cmp ) ) continue;
      if ( open_room_cmp.findIntersection( player_pos_cmp ) ) continue;

      Factory::Crypt::destroy_crypt_lever( reg(), lever_entt );
    }
  }
}

void CryptSystem::remove_chest_open_rooms( const Cmp::Position &player_pos_cmp )
{

  for ( auto [chest_entt, chest_cmp, chest_pos_cmp] : reg().view<Cmp::Crypt::Chest, Cmp::Position>().each() )
  {
    bool player_in_same_room = false;
    for ( auto [open_room_entt, open_room_cmp] : reg().view<Cmp::Crypt::RoomOpen>().each() )
    {
      if ( not open_room_cmp.findIntersection( player_pos_cmp ) ) continue; // player not in this room

      // expand room bounds by 1 tile to include border positions where chests are placed
      // sf::FloatRect expanded_room(
      //     sf::Vector2f{ open_room_cmp.position.x - Constants::kGridSizePxF.x, open_room_cmp.position.y - Constants::kGridSizePxF.y },
      //     sf::Vector2f{ open_room_cmp.size.x + Constants::kGridSizePxF.x * 2.f, open_room_cmp.size.y + Constants::kGridSizePxF.y * 2.f } );

      auto expanded_room = Cmp::RectBounds::expanded( open_room_cmp, 1 );

      if ( expanded_room.findIntersection( chest_pos_cmp ) )
      {
        player_in_same_room = true;
        break;
      }
    }

    if ( not player_in_same_room ) { Factory::Crypt::destroy_crypt_chest( reg(), chest_entt ); }
  }
}

void CryptSystem::remove_all_levers()
{
  for ( auto [lever_entt, lever_cmp, lever_pos_cmp] : reg().view<Cmp::Crypt::Lever, Cmp::Position>().each() )

  {
    Factory::Crypt::destroy_crypt_lever( reg(), lever_entt );
  }
}

void CryptSystem::remove_all_chests()
{
  for ( auto [chest_entt, chest_cmp, chest_pos_cmp] : reg().view<Cmp::Crypt::Chest, Cmp::Position>().each() )
  {
    Factory::Crypt::destroy_crypt_chest( reg(), chest_entt );
    if ( auto player_navmesh = m_player_navmesh.lock() ) { player_navmesh->remove( chest_entt, chest_pos_cmp ); }
  }
}

void CryptSystem::spawn_npc_in_open_rooms()
{
  auto open_room_view = reg().view<Cmp::Crypt::RoomOpen>();
  std::vector<Cmp::Crypt::RoomOpen> open_room_list;

  // Collect all open rooms
  for ( auto [open_room_entt, open_room_cmp] : open_room_view.each() )
  {
    open_room_list.push_back( open_room_cmp );
  }

  // Early return if not enough rooms
  if ( open_room_list.size() < 2 )
  {
    SPDLOG_WARN( "Not enough open rooms to spawn 2 NPCs. Available rooms: {}", open_room_list.size() );
    return;
  }

  // Spawn user specified number of NPCs or the max number of rooms: whichever is smallest.
  const int npcs_to_spawn = std::min( Sys::PersistSystem::get<Cmp::Persist::CryptNpcSpawnCount>( reg() ).get_value(),
                                      static_cast<unsigned short>( open_room_list.size() ) );

  for ( int r = 0; r < npcs_to_spawn; r++ )
  {
    // Select a random room from remaining rooms
    Cmp::RandomInt room_picker( 0, static_cast<int>( open_room_list.size() ) - 1 );
    std::size_t selected_idx = room_picker.gen();

    // Extract the selected room
    auto extracted_open_room_cmp = std::move( open_room_list[selected_idx] );

    if ( selected_idx < open_room_list.size() - 1 )
    {
      // Remove selected room using swap and pop to maintain efficiency
      open_room_list[selected_idx] = std::move( open_room_list.back() );
    }
    open_room_list.pop_back();

    // Spawn NPC in the selected room
    auto spawn_position = Utils::snap_to_grid( extracted_open_room_cmp.getCenter() );
    auto position_entity = reg().create();
    Cmp::Position position_cmp = reg().emplace<Cmp::Position>( position_entity, spawn_position, Constants::kGridSizePxF );
    [[maybe_unused]] Cmp::ZOrderValue zorder_cmp = reg().emplace<Cmp::ZOrderValue>( position_entity, position_cmp.position.y );
    Factory::Npc::create_npc( reg(), position_entity, "npc.priest" );

    SPDLOG_DEBUG( "Spawned NPC {} at position ({}, {})", r + 1, spawn_position.x, spawn_position.y );
  }
}

std::pair<entt::entity, Cmp::Crypt::RoomStart &> CryptSystem::get_crypt_room_start()
{
  auto start_room_view = reg().view<Cmp::Crypt::RoomStart>();
  if ( start_room_view.front() == entt::null ) throw std::runtime_error( "CryptSystem::get_crypt_room_start - Unable to get Cmp::Crypt::RoomStart" );
  return { start_room_view.front(), reg().get<Cmp::Crypt::RoomStart>( start_room_view.front() ) };
}

std::pair<entt::entity, Cmp::Crypt::RoomEnd &> CryptSystem::get_crypt_room_end()
{
  auto end_room_view = reg().view<Cmp::Crypt::RoomEnd>();
  if ( end_room_view.front() == entt::null ) throw std::runtime_error( "CryptSystem::get_crypt_room_end - Unable to get Cmp::Crypt::RoomEnd" );
  return { end_room_view.front(), reg().get<Cmp::Crypt::RoomEnd>( end_room_view.front() ) };
}

} // namespace Game::Sys