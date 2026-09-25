#include <Audio/SoundBank.hpp>
#include <Components/Crypt/Chest.hpp>
#include <Components/Crypt/PassageBlock.hpp>
#include <Components/Crypt/PassageDoor.hpp>
#include <Components/Crypt/RoomClosed.hpp>
#include <Components/Crypt/RoomEnd.hpp>
#include <Components/Crypt/RoomOpen.hpp>
#include <Components/Crypt/RoomStart.hpp>
#include <Components/FootStepAlpha.hpp>
#include <Components/FootStepTimer.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Random.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/SpawnArea.hpp>
#include <Components/UUID.hpp>
#include <Events/PassageEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Factory/CryptFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <SceneControl/SceneData.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/ProcGen/PassageSystem.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Player.hpp>

#include <Utils/Utils.hpp>
#include <map>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace Game::Sys
{

PassageSystem::PassageSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : Game::Sys::BaseSystem( reg, window, sprite_factory, sound_bank )

{
  std::ignore = get_systems_event_queue().sink<Events::PassageEvent>().connect<&PassageSystem::on_passage_event>( this );
}

void PassageSystem::update( [[maybe_unused]] sf::Time dt )
{
  if ( m_connect_all_rooms ) { create_cached_passages(); }
}

void PassageSystem::init_nav_mesh( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh )
{
  m_npc_navmesh = npc_navmesh;
  m_passage_algos.cache_wall_components( reg() );
}

void PassageSystem::on_passage_event( Events::PassageEvent &event )
{
  Scene::SceneMapSharedPtr crypt_scene_data = m_crypt_scene_data.lock();
  if ( not crypt_scene_data ) throw std::runtime_error( "Unable to lock Scene::SceneConfigSharedPtr" );
  auto [map_size_grid, map_size_pixel] = crypt_scene_data->map_size();

  switch ( event.type )
  {
    case Events::PassageEvent::Type::REMOVE_PASSAGES:
      m_passage_algos.reset();
      fill_all_passages();
      remove_all_passage_blocks();
      m_uncached_passage_list.clear();
      break;
    case Events::PassageEvent::Type::OPEN_PASSAGES:
      open_passages();
      break;
    case Events::PassageEvent::Type::CONNECT_START_TO_OPENROOMS:
      connect_start_and_open_rooms_passages( event.entity );
      break;
    case Events::PassageEvent::Type::CONNECT_OCCUPIED_TO_OPENROOMS:
      connect_occupied_and_open_room_passages();
      break;
    case Events::PassageEvent::Type::CONNECT_OCCUPIED_TO_ENDROOM:
      connect_occupied_and_end_room_passages( reg(), event.entity, map_size_pixel );
      tidy_passage_blocks();
      create_uncached_passages();
      break;
    case Events::PassageEvent::Type::CACHE_ALL_ROOM_CONNECTIONS:
      m_region_idx = 0;
      cache_all_room_connections();
      break;
    case Events::PassageEvent::Type::CONNECT_ALL_ROOMS:
      m_connect_all_rooms = true;
      break;
    case Events::PassageEvent::Type::ADD_SPIKE_TRAPS:
      add_spike_traps();
      break;
  }
}

void PassageSystem::open_passages()
{
  //
  empty_open_passages();
}

void PassageSystem::add_spike_traps()
{
  auto passage_picker = Cmp::RandomInt( 0, static_cast<int>( m_passage_algos.get_current_passage_id() ) );
  // static int max_num_spike_traps = 3;
  std::set<int> passage_ids_used;

  auto pblock_view = reg().view<Cmp::Crypt::PassageBlock>();
  for ( auto [pblock_entt, pblock_cmp] : pblock_view.each() )
  {
    if ( passage_ids_used.contains( static_cast<int>( pblock_cmp.m_passage_id ) ) ) continue;
    passage_ids_used.insert( static_cast<int>( pblock_cmp.m_passage_id ) );
    Factory::Crypt::add_spike_trap( reg(), pblock_entt, static_cast<int>( pblock_cmp.m_passage_id ) );
  }
}

void PassageSystem::connect_start_and_open_rooms_passages( entt::entity start_room_entt )
{
  if ( start_room_entt == entt::null )
  {
    SPDLOG_WARN( "start_room_entt is null" );
    return;
  }
  auto *start_room_cmp = reg().try_get<Cmp::Crypt::RoomStart>( start_room_entt );
  if ( not start_room_cmp )
  {
    SPDLOG_WARN( "start_room_cmp is null" );
    return;
  }

  Scene::SceneMapSharedPtr crypt_scene_data = m_crypt_scene_data.lock();
  if ( not crypt_scene_data ) throw std::runtime_error( "Unable to lock Scene::SceneConfigSharedPtr" );
  auto [map_size_grid, map_size_pixel] = crypt_scene_data->map_size();

  const auto start_room_right_pos_x = start_room_cmp->position.x + start_room_cmp->size.x;

  // divide the gamrarea into 3 quadrants - again there are only three because startroom is southern most position in the game area
  auto west_quad = sf::FloatRect( { 0.f, 0.f }, { start_room_cmp->position.x, map_size_pixel.y } );
  auto east_quad = sf::FloatRect( { start_room_right_pos_x, 0.f }, { map_size_pixel.x - ( start_room_right_pos_x ), map_size_pixel.y } );
  auto north_quad = sf::FloatRect( { 0.f, 0.f }, { map_size_pixel.x, start_room_cmp->position.y } );

  std::vector<std::pair<Cmp::Crypt::CryptPassageDirection, sf::FloatRect>> quadrants = { { Cmp::Crypt::CryptPassageDirection::WEST, west_quad },
                                                                                         { Cmp::Crypt::CryptPassageDirection::EAST, east_quad },
                                                                                         { Cmp::Crypt::CryptPassageDirection::NORTH, north_quad } };

  for ( auto &[direction, qaudrant] : quadrants )
  {
    auto distances = find_room_distances<Cmp::Crypt::RoomOpen>( start_room_cmp->m_connectors[direction], qaudrant, { start_room_entt } );
    auto passage_blocks = find_passages<Cmp::Crypt::RoomOpen>( start_room_cmp->m_connectors[direction], distances, ProcGen::WalkingType::DRUNK,
                                                               map_size_pixel, ProcGen::OnePassagePerTargetRoom::YES,
                                                               ProcGen::AllowDuplicatePassages::NO );
    m_uncached_passage_list.insert( m_uncached_passage_list.begin(), passage_blocks.begin(), passage_blocks.end() );
  }

  create_uncached_passages();
  tidy_passage_blocks();
}

void PassageSystem::connect_occupied_and_open_room_passages()
{
  Scene::SceneMapSharedPtr crypt_scene_data = m_crypt_scene_data.lock();
  if ( not crypt_scene_data ) throw std::runtime_error( "Unable to lock Scene::SceneConfigSharedPtr" );

  auto [map_size_grid, map_size_pixel] = crypt_scene_data->map_size();

  // find the open room that the player is in (if any)
  auto open_room_view = reg().view<Cmp::Crypt::RoomOpen>();
  for ( auto [open_room_entt, open_room_cmp] : open_room_view.each() )
  {
    if ( not Utils::Player::get_position( reg() ).findIntersection( open_room_cmp ) ) continue;

    auto &occupied_room_cmp = open_room_cmp;

    const auto current_room_right_pos_x = occupied_room_cmp.position.x + occupied_room_cmp.size.x;
    const auto current_room_bottom_pos_y = occupied_room_cmp.position.y + occupied_room_cmp.size.y;

    // divide the gamrarea into 4 quadrants
    auto west_quad = sf::FloatRect( { 0.f, 0.f }, { occupied_room_cmp.position.x, map_size_pixel.y } );
    auto east_quad = sf::FloatRect( { current_room_right_pos_x, 0.f }, { map_size_pixel.x - ( current_room_right_pos_x ), map_size_pixel.y } );
    auto north_quad = sf::FloatRect( { 0.f, 0.f }, { map_size_pixel.x, occupied_room_cmp.position.y } );
    auto south_quad = sf::FloatRect( { occupied_room_cmp.position.x, current_room_bottom_pos_y },
                                     { current_room_right_pos_x, map_size_pixel.y - ( current_room_bottom_pos_y ) } );

    std::vector<std::pair<Cmp::Crypt::CryptPassageDirection, sf::FloatRect>> quadrants = { { Cmp::Crypt::CryptPassageDirection::WEST, west_quad },
                                                                                           { Cmp::Crypt::CryptPassageDirection::EAST, east_quad },
                                                                                           { Cmp::Crypt::CryptPassageDirection::NORTH, north_quad },
                                                                                           { Cmp::Crypt::CryptPassageDirection::SOUTH, south_quad } };

    for ( auto &[direction, qaudrant] : quadrants )
    {
      auto distances = find_room_distances<Cmp::Crypt::RoomOpen>( occupied_room_cmp.m_connectors[direction], qaudrant, { open_room_entt } );
      auto passage_blocks = find_passages<Cmp::Crypt::RoomOpen>( occupied_room_cmp.m_connectors[direction], distances, ProcGen::WalkingType::DRUNK,
                                                                 map_size_pixel, ProcGen::OnePassagePerTargetRoom::YES,
                                                                 ProcGen::AllowDuplicatePassages::NO );
      m_uncached_passage_list.insert( m_uncached_passage_list.begin(), passage_blocks.begin(), passage_blocks.end() );
    }
    create_uncached_passages();
    tidy_passage_blocks();
  }
}

void PassageSystem::connect_occupied_and_end_room_passages( entt::registry &reg, entt::entity end_room_entt, sf::Vector2f map_size_pixel )
{
  std::vector<Cmp::Crypt::PassageBlock> passage_block_list;
  if ( end_room_entt == entt::null )
  {
    SPDLOG_WARN( "End room entt is null" );
    return;
  }
  auto *crypt_end_room_cmp = reg.try_get<Cmp::Crypt::RoomEnd>( end_room_entt );
  if ( not crypt_end_room_cmp )
  {
    SPDLOG_WARN( "end room cmp is null" );
    return;
  }

  auto open_room_view = reg.view<Cmp::Crypt::RoomOpen>();
  for ( auto [open_room_entt, open_room_cmp] : open_room_view.each() )
  {
    if ( not Utils::Player::get_position( reg ).findIntersection( open_room_cmp ) ) continue;
    auto &occupied_room_cmp = open_room_cmp;

    // no need to search for suitable target, we already have it
    auto current_passage_door = occupied_room_cmp.m_connectors[Cmp::Crypt::CryptPassageDirection::NORTH];
    m_passage_algos.increment_passage_id();

    std::vector<Cmp::Crypt::PassageBlock> passage_blocks = m_passage_algos.create_drunken_walk( reg, current_passage_door, *crypt_end_room_cmp,
                                                                                                map_size_pixel, { open_room_entt, end_room_entt } );

    m_uncached_passage_list.insert( m_uncached_passage_list.begin(), passage_blocks.begin(), passage_blocks.end() );
  }
}

void PassageSystem::cache_all_room_connections()
{

  Scene::SceneMapSharedPtr crypt_scene_data = m_crypt_scene_data.lock();
  if ( not crypt_scene_data ) throw std::runtime_error( "Unable to lock Scene::SceneConfigSharedPtr" );

  auto [map_size_grid, map_size_pixel] = crypt_scene_data->map_size();
  const auto world_area = sf::FloatRect( { 0, 0 }, map_size_pixel );

  // divide the world into regions using the array size
  float region_height = world_area.size.y / static_cast<float>( m_cached_passage_list.size() );
  float region_width = world_area.size.x;
  for ( auto [idx, it] : std::views::enumerate( m_cached_passage_list ) )
  {
    auto new_region = sf::FloatRect( { 0.f, static_cast<float>( idx ) * region_height }, { region_width, region_height } );
    it = ProcGen::PassageCachedRegions<40>::BlockRegion{ .region = new_region, .blocklist = {} };
    SPDLOG_DEBUG( "Created cached region {},{} {},{}", new_region.position.x, new_region.position.y, new_region.size.x, new_region.size.y );
  }

  // Do this first to guarantee success otherwise player cannot leave.
  // This also marks exactly one closed room as all_doors_used — that becomes the BFS root.
  connect_end_room_to_nearest_closed_room();

  // The closed room the end room just connected to is guaranteed reachable from the exit.
  // Use it as the root for graph connectivity checks below.
  entt::entity bfs_root = entt::null;
  for ( auto [closed_room_entt, closed_room_cmp] : reg().view<Cmp::Crypt::RoomClosed>().each() )
  {
    if ( closed_room_cmp.are_all_doors_used() )
    {
      bfs_root = closed_room_entt;
      break;
    }
  }

  std::vector<Cmp::Crypt::CryptPassageDirection> directions = { Cmp::Crypt::CryptPassageDirection::WEST, Cmp::Crypt::CryptPassageDirection::EAST,
                                                                Cmp::Crypt::CryptPassageDirection::NORTH, Cmp::Crypt::CryptPassageDirection::SOUTH };

  // Shared helper: cache passage blocks and advance the passage ID.
  auto cache_blocks = [&]( const std::vector<Cmp::Crypt::PassageBlock> &passage_blocks )
  {
    m_passage_algos.increment_passage_id();
    for ( const auto &passage_block_cmp : passage_blocks )
    {
      for ( auto &[region, blocklist] : m_cached_passage_list )
      {
        if ( not sf::FloatRect( passage_block_cmp, Constants::kGridSizePxF ).findIntersection( region ) ) continue;
        blocklist.push_back( passage_block_cmp );
      }
    }
  };

  // First pass: sparse connections — each room can only be targeted once, preserving dead-end rooms.
  // Inline the find_passages loop so the target entity is visible here and can be added to the
  // adjacency graph. Using find_passages would hide the target entity inside the template.
  std::map<entt::entity, std::set<entt::entity>> adjacency;
  if ( bfs_root != entt::null ) adjacency[bfs_root]; // ensure root has an entry even with no extra edges

  for ( auto [closed_room_entt, closed_room_cmp] : reg().view<Cmp::Crypt::RoomClosed>().each() )
  {
    for ( auto &direction : directions )
    {
      auto distances = find_room_distances<Cmp::Crypt::RoomClosed>( closed_room_cmp.m_connectors[direction], world_area, { closed_room_entt } );
      while ( not distances.empty() )
      {
        auto [dist_val, target_entt] = distances.top();
        distances.pop();
        if ( not reg().valid( target_entt ) ) continue;
        auto *target_cmp = reg().try_get<Cmp::Crypt::RoomClosed>( target_entt );
        if ( not target_cmp ) continue;

        auto passage_list = m_passage_algos.create_drunken_walk( reg(), closed_room_cmp.m_connectors[direction], *target_cmp, map_size_pixel,
                                                                 { target_entt }, ProcGen::AllowDuplicatePassages::NO );
        if ( not passage_list.empty() )
        {
          target_cmp->set_all_doors_used( true );
          adjacency[closed_room_entt].insert( target_entt );
          adjacency[target_entt].insert( closed_room_entt ); // passages are traversable both ways
          cache_blocks( passage_list );
          break;
        }
      }
    }
  }

  // BFS from the BFS root to find every room that is genuinely reachable from the exit.
  // The earlier heuristic ("has any connection") missed disconnected sub-graphs where two
  // isolated rooms connect to each other but not to the main graph (X→Y, Y→X cycle).
  std::set<entt::entity> reachable;
  if ( bfs_root != entt::null )
  {
    std::vector<entt::entity> bfs_queue = { bfs_root };
    reachable.insert( bfs_root );
    for ( std::size_t i = 0; i < bfs_queue.size(); ++i )
    {
      for ( auto neighbor : adjacency[bfs_queue[i]] )
      {
        if ( reachable.contains( neighbor ) ) continue;
        reachable.insert( neighbor );
        bfs_queue.push_back( neighbor );
      }
    }
  }

  // Second pass: rooms absent from the BFS-reachable set are disconnected islands (soft-lock).
  // Connect each to the nearest reachable room; DOGLEG fallback handles edge-room walk failures.
  std::vector<entt::entity> isolated_rooms;
  for ( auto [closed_room_entt, closed_room_cmp] : reg().view<Cmp::Crypt::RoomClosed>().each() )
  {
    if ( not reachable.contains( closed_room_entt ) ) isolated_rooms.push_back( closed_room_entt );
  }

  if ( not isolated_rooms.empty() )
  {
    SPDLOG_WARN( "{} isolated rooms detected, adding minimal fallback connections", isolated_rooms.size() );
    for ( auto [e, cmp] : reg().view<Cmp::Crypt::RoomClosed>().each() )
      cmp.set_all_doors_used( false );

    auto build_reachable_distances = [&]( const Cmp::Crypt::PassageDoor &door, entt::entity exclude ) -> ProcGen::MidPointDistanceQueue
    {
      ProcGen::MidPointDistanceQueue dist;
      for ( auto [room_entt, room_cmp] : reg().view<Cmp::Crypt::RoomClosed>().each() )
      {
        if ( room_entt == exclude ) continue;
        if ( not reachable.empty() && not reachable.contains( room_entt ) ) continue;
        dist.emplace( Utils::Maths::getEuclideanDistance( door, room_cmp.getCenter() ), room_entt );
      }
      return dist;
    };

    for ( auto isolated_entt : isolated_rooms )
    {
      // Skip rooms that became reachable by propagation from a previously connected isolated room.
      if ( reachable.contains( isolated_entt ) ) continue;

      auto *isolated_cmp = reg().try_get<Cmp::Crypt::RoomClosed>( isolated_entt );
      if ( not isolated_cmp ) continue;
      bool connected = false;
      for ( auto &direction : directions )
      {
        auto dist = build_reachable_distances( isolated_cmp->m_connectors[direction], isolated_entt );
        auto passage_blocks = find_passages<Cmp::Crypt::RoomClosed>( isolated_cmp->m_connectors[direction], dist, ProcGen::WalkingType::DRUNK,
                                                                     map_size_pixel, ProcGen::OnePassagePerTargetRoom::NO,
                                                                     ProcGen::AllowDuplicatePassages::NO );
        if ( not passage_blocks.empty() )
        {
          cache_blocks( passage_blocks );
          connected = true;
          break;
        }

        // Drunken walk can fail for edge rooms (initial orthogonal steps exit map bounds).
        // Dog-leg has no such constraint and creates a deterministic L-shaped path.
        dist = build_reachable_distances( isolated_cmp->m_connectors[direction], isolated_entt );
        passage_blocks = find_passages<Cmp::Crypt::RoomClosed>( isolated_cmp->m_connectors[direction], dist, ProcGen::WalkingType::DOGLEG,
                                                                map_size_pixel, ProcGen::OnePassagePerTargetRoom::NO,
                                                                ProcGen::AllowDuplicatePassages::NO );
        if ( not passage_blocks.empty() )
        {
          cache_blocks( passage_blocks );
          connected = true;
          break;
        }
      }
      if ( connected )
      {
        // Propagate reachability through first-pass adjacency edges. Any room that was already
        // connected to this room in the first pass is now reachable too, so it doesn't need its
        // own second-pass passage — one bridge per disconnected component is enough.
        reachable.insert( isolated_entt );
        std::vector<entt::entity> propagate_queue = { isolated_entt };
        for ( std::size_t i = 0; i < propagate_queue.size(); ++i )
        {
          for ( auto neighbor : adjacency[propagate_queue[i]] )
          {
            if ( reachable.contains( neighbor ) ) continue;
            reachable.insert( neighbor );
            propagate_queue.push_back( neighbor );
          }
        }
      }
      else { SPDLOG_ERROR( "Could not connect isolated room {} — dungeon may be soft-locked", entt::to_integral( isolated_entt ) ); }
    }
  }

  SPDLOG_DEBUG( "BlockRegion count {}", m_cached_passage_list.size() );

  // tidyPassageBlocks();
}

void PassageSystem::connect_end_room_to_nearest_closed_room()
{
  auto end_room_view = reg().view<Cmp::Crypt::RoomEnd>();
  if ( end_room_view->empty() )
  {
    SPDLOG_WARN( "no end room found" );
    return;
  }

  auto end_room_entt = *end_room_view.begin();
  auto &end_room_cmp = end_room_view.get<Cmp::Crypt::RoomEnd>( end_room_entt );

  Scene::SceneMapSharedPtr crypt_scene_data = m_crypt_scene_data.lock();
  if ( not crypt_scene_data )
  {
    SPDLOG_ERROR( "crypt_scene_data is null" );
    return;
  }
  auto [map_size_grid, map_size_pixel] = crypt_scene_data->map_size();
  const auto world_area = sf::FloatRect( { 0.f, 0.f }, map_size_pixel );

  SPDLOG_DEBUG( "connecting end room {} to nearest closed room", static_cast<uint32_t>( end_room_entt ) );

  // Try only south door as RoomEnd only appears at the top of the game area, stop as soon as one walk succeeds
  const std::array directions = { Cmp::Crypt::CryptPassageDirection::SOUTH };
  for ( auto direction : directions )
  {
    auto distances = find_room_distances<Cmp::Crypt::RoomClosed>( end_room_cmp.m_connectors[direction], world_area, { end_room_entt } );
    auto passage_blocks = find_passages<Cmp::Crypt::RoomClosed>( end_room_cmp.m_connectors[direction], distances, ProcGen::WalkingType::DRUNK,
                                                                 map_size_pixel, ProcGen::OnePassagePerTargetRoom::YES,
                                                                 ProcGen::AllowDuplicatePassages::NO );
    if ( not passage_blocks.empty() ) m_passage_algos.increment_passage_id();

    for ( auto &passage_block_cmp : passage_blocks )
    {
      for ( auto &[region, blocklist] : m_cached_passage_list )
      {
        if ( not sf::FloatRect( passage_block_cmp, Constants::kGridSizePxF ).findIntersection( region ) ) continue;
        blocklist.push_back( passage_block_cmp );
      }
    }
  }
}

void PassageSystem::create_uncached_passages()
{
  for ( auto &passage_block_cmp : m_uncached_passage_list )
  {
    auto entt = reg().create();
    reg().emplace<Cmp::Crypt::PassageBlock>( entt, passage_block_cmp );
    m_passage_block_grid.insert( entt, Cmp::Position( passage_block_cmp, Constants::kGridSizePxF ) );
  }
}

void PassageSystem::create_cached_passages()
{

  if ( m_region_idx >= m_cached_passage_list.size() )
  {
    m_connect_all_rooms = false;
    open_passages();
    add_spike_traps();
    return;
  }

  auto [region, blocklist] = m_cached_passage_list.get( m_region_idx );
  SPDLOG_DEBUG( "Spawning region {} with {} blocks", m_region_idx, blocklist.size() );
  for ( auto &block : blocklist )
  {
    auto entt = reg().create();
    reg().emplace<Cmp::Crypt::PassageBlock>( entt, block );
    m_passage_block_grid.insert( entt, Cmp::Position( block, Constants::kGridSizePxF ) );
  }
  m_region_idx++;
}

template <typename ROOMTYPE>
ProcGen::MidPointDistanceQueue PassageSystem::find_room_distances( Cmp::Crypt::PassageDoor &start_passage_door, const sf::FloatRect &search_quadrant,
                                                                   std::set<entt::entity> exclude_entts )
{

  ProcGen::MidPointDistanceQueue pqueue;
  for ( auto [other_room_entt, other_room_cmp] : reg().view<ROOMTYPE>().each() )
  {
    if ( exclude_entts.contains( other_room_entt ) ) continue;
    if ( not other_room_cmp.findIntersection( search_quadrant ) ) continue;
    if ( other_room_cmp.are_all_doors_used() ) continue;
    auto distance = Utils::Maths::getEuclideanDistance( start_passage_door, other_room_cmp.getCenter() );
    SPDLOG_DEBUG( "Found room {} at distance {}", static_cast<uint32_t>( other_room_entt ), distance );
    pqueue.push( { distance, other_room_entt } );
  }

  return pqueue;
}
template ProcGen::MidPointDistanceQueue PassageSystem::find_room_distances<Cmp::Crypt::RoomOpen>( Cmp::Crypt::PassageDoor &, const sf::FloatRect &,
                                                                                                  std::set<entt::entity> );
template ProcGen::MidPointDistanceQueue PassageSystem::find_room_distances<Cmp::Crypt::RoomClosed>( Cmp::Crypt::PassageDoor &, const sf::FloatRect &,
                                                                                                    std::set<entt::entity> );

template <typename ROOMTYPE>
std::vector<Cmp::Crypt::PassageBlock> PassageSystem::find_passages( Cmp::Crypt::PassageDoor &start_passage_door,
                                                                    ProcGen::MidPointDistanceQueue &dist_pqueue, ProcGen::WalkingType walktype,
                                                                    sf::Vector2f map_size_pixel, ProcGen::OnePassagePerTargetRoom passage_limit,
                                                                    ProcGen::AllowDuplicatePassages duplicates_policy )
{
  std::vector<Cmp::Crypt::PassageBlock> final_passage_list;

  // process the distance list, one room at a time
  while ( not dist_pqueue.empty() )
  {
    auto nearest_other_room_entt = dist_pqueue.top().second;
    dist_pqueue.pop();
    if ( not reg().valid( nearest_other_room_entt ) ) continue;

    auto *other_room_bounds = reg().try_get<ROOMTYPE>( nearest_other_room_entt );
    if ( not other_room_bounds ) continue;

    std::vector<Cmp::Crypt::PassageBlock> passage_list;

    // try to create a room-to-room pathway
    if ( walktype == ProcGen::WalkingType::DRUNK )
    {
      passage_list = m_passage_algos.create_drunken_walk( reg(), start_passage_door, *other_room_bounds, map_size_pixel, { nearest_other_room_entt },
                                                          duplicates_policy );
    }
    else { passage_list = m_passage_algos.create_dog_leg( reg(), start_passage_door, *other_room_bounds, duplicates_policy ); }

    // if the pathway was successfull in reaching goal
    if ( not passage_list.empty() )
    {
      auto *room_cmp = reg().try_get<ROOMTYPE>( nearest_other_room_entt );
      if ( passage_limit == ProcGen::OnePassagePerTargetRoom::YES && room_cmp ) { room_cmp->set_all_doors_used( true ); }

      final_passage_list.insert( final_passage_list.end(), passage_list.begin(), passage_list.end() );

      break;
    }
  }
  return final_passage_list;
}

template std::vector<Cmp::Crypt::PassageBlock>
PassageSystem::find_passages<Cmp::Crypt::RoomOpen>( Cmp::Crypt::PassageDoor &, ProcGen::MidPointDistanceQueue &, ProcGen::WalkingType, sf::Vector2f,
                                                    ProcGen::OnePassagePerTargetRoom, ProcGen::AllowDuplicatePassages );

template std::vector<Cmp::Crypt::PassageBlock>
PassageSystem::find_passages<Cmp::Crypt::RoomClosed>( Cmp::Crypt::PassageDoor &, ProcGen::MidPointDistanceQueue &, ProcGen::WalkingType, sf::Vector2f,
                                                      ProcGen::OnePassagePerTargetRoom, ProcGen::AllowDuplicatePassages );

/// PRIVATE FUNCTIONS

void PassageSystem::remove_all_passage_blocks()
{
  std::vector<entt::entity> passage_block_remove_list;

  for ( auto [entt, block_cmp] : reg().view<Cmp::Crypt::PassageBlock>().each() )
  {
    passage_block_remove_list.push_back( entt );
  }

  // Remove Cmp::CryptPassageBlocks safely
  for ( auto entt : passage_block_remove_list )
  {
    reg().remove<Cmp::Crypt::PassageBlock>( entt );
    reg().destroy( entt );
  }
  m_passage_block_grid.clear();
}

void PassageSystem::empty_open_passages()
{
  PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = m_npc_navmesh.lock();
  if ( not pathfinding_navmesh ) return;
  auto reserved_sm = m_reserved_sm.lock();

  std::vector<std::pair<entt::entity, Cmp::Position>> obstacles_to_remove;
  std::vector<std::pair<entt::entity, Cmp::Position>> chests_to_remove;

  for ( auto [pos_entt, pos_cmp] : reg().view<Cmp::Position>().each() )
  {
    if ( m_passage_block_grid.at( pos_cmp ).empty() ) continue;
    if ( reg().any_of<Cmp::Obstacle>( pos_entt ) ) obstacles_to_remove.emplace_back( pos_entt, pos_cmp );
    if ( reg().any_of<Cmp::Crypt::Chest>( pos_entt ) ) chests_to_remove.emplace_back( pos_entt, pos_cmp );
  }

  for ( auto &[entt, pos_cmp] : obstacles_to_remove )
  {
    Factory::Obstacle::remove_obstacle( reg(), entt, Factory::Obstacle::DeleteExtras::Yes, reserved_sm );
    pathfinding_navmesh->insert( entt, pos_cmp );
  }
  for ( auto &[entt, pos_cmp] : chests_to_remove )
  {
    Factory::Crypt::destroy_crypt_chest( reg(), entt );
    pathfinding_navmesh->insert( entt, pos_cmp );
  }
}

void PassageSystem::fill_all_passages()
{
  // Position view iteration is fully complete
  const Sprites::SpriteSheet &ss_main = m_sprite_factory.get_spritesheet_by_type( "sprite.crypt.wall.int.main" );
  const Sprites::SpriteSheet &ss_cap = m_sprite_factory.get_spritesheet_by_type( "sprite.crypt.wall.int.cap" );
  PathFinding::SpatialHashGridSharedPtr pathfinding_navmesh = m_npc_navmesh.lock();
  auto reserved_sm = m_reserved_sm.lock();

  for ( auto [pos_entt, pos_cmp] : reg().view<Cmp::Position>().each() )
  {
    if ( reg().any_of<Cmp::FootStepTimer, Cmp::FootStepAlpha, Cmp::Direction>( pos_entt ) ) continue;
    if ( reg().all_of<Cmp::Obstacle>( pos_entt ) ) continue;
    if ( reg().all_of<Cmp::UUID>( pos_entt ) ) continue; // skip cap entities (no Obstacle but already decorated)
    // spawn tiles are authored wider than Cmp::Crypt::RoomStart's bounds - never wall over them
    if ( reg().any_of<Cmp::SpawnArea>( pos_entt ) ) continue;

    // m_passage_block_grid is the authoritative "this position must become a wall now" signal -
    // don't also gate on the reserved-positions grid here: an unrelated wall's cap sprite (one
    // tile above that wall) can land on this exact position and phantom-reserve it, and since
    // this pass is the only place that ever fills a tracked passage position back in, skipping it
    // would leave a walkable gap here permanently once m_passage_block_grid is cleared afterward.
    if ( m_passage_block_grid.at( pos_cmp ).empty() ) continue;

    Factory::Obstacle::add_obstacle( reg(), pos_entt );
    Factory::Obstacle::decorate_obstacle( reg(), pos_entt, pos_cmp, ss_main, 0, pos_cmp.y() + ss_main.get_zorder( 0 ) );

    auto uuid = Cmp::UUID::generate();
    reg().emplace_or_replace<Cmp::UUID>( pos_entt, uuid );

    auto cap_entt = reg().create();
    Cmp::Position cap_position( { pos_cmp.x(), pos_cmp.y() - pos_cmp.size.y }, pos_cmp.size );
    reg().emplace_or_replace<Cmp::Position>( cap_entt, cap_position );
    Factory::Obstacle::decorate_obstacle( reg(), cap_entt, cap_position, ss_cap, 0, pos_cmp.y() + ss_cap.get_zorder( 0 ), false );
    if ( reserved_sm ) reserved_sm->insert( cap_entt, cap_position );
    reg().emplace_or_replace<Cmp::UUID>( cap_entt, uuid );
    Factory::Obstacle::add_obstacle_cap( reg(), cap_entt );

    if ( pathfinding_navmesh ) pathfinding_navmesh->remove( pos_entt, pos_cmp );
  }

  // Player squish check — done once after all walls are placed - except if player has extra life.
  if ( Utils::scene_setting<Cmp::SceneSettings::CollisionDetection>( reg() ).enabled )
  {
    if ( not m_passage_block_grid.at( Utils::Player::get_position( reg() ) ).empty() )
    {

      get_systems_event_queue().enqueue(
          Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::SQUISHED, Utils::Player::get_position( reg() ) ) );
    }
  }
}

void PassageSystem::tidy_passage_blocks( bool include_closed_rooms )
{
  for ( auto [pblock_entt, pblock_cmp] : reg().view<Cmp::Crypt::PassageBlock>().each() )
  {
    auto pblock_cmp_rect = sf::FloatRect( pblock_cmp, Constants::kGridSizePxF );
    // Captured by value before any removal — reg().remove() below can invalidate the pblock_cmp
    // reference (swap-and-pop within the component pool), so the grid position must be read first.
    auto pblock_position = Cmp::Position( pblock_cmp, Constants::kGridSizePxF );

    // Removing the PassageBlock component alone leaves the tile marked as a passage in
    // m_passage_block_grid — that stale entry then gets walled in by fill_all_passages() and can
    // squish the player standing on what is actually room floor. Keep the grid in sync.
    auto untrack = [&]()
    {
      reg().remove<Cmp::Crypt::PassageBlock>( pblock_entt );
      m_passage_block_grid.remove( pblock_entt, pblock_position );
    };

    // open rooms
    for ( auto [open_room_entt, open_room_cmp] : reg().view<Cmp::Crypt::RoomOpen>().each() )
    {
      if ( pblock_cmp_rect.findIntersection( open_room_cmp ) ) untrack();
    }

    // closed rooms - this can interfere with passage creation so normal usescases don't need it
    if ( include_closed_rooms )
    {
      for ( auto [closed_room_entt, closed_room_cmp] : reg().view<Cmp::Crypt::RoomClosed>().each() )
      {
        if ( pblock_cmp_rect.findIntersection( closed_room_cmp ) ) untrack();
      }
    }

    // start rooms
    for ( auto [start_room_entt, start_room_cmp] : reg().view<Cmp::Crypt::RoomStart>().each() )
    {
      if ( pblock_cmp_rect.findIntersection( start_room_cmp ) ) untrack();
    }

    // end rooms
    for ( auto [end_room_entt, end_room_cmp] : reg().view<Cmp::Crypt::RoomEnd>().each() )
    {
      if ( pblock_cmp_rect.findIntersection( end_room_cmp ) ) untrack();
    }
  }
}

} // namespace Game::Sys