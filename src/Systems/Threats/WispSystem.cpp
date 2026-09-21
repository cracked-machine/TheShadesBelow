#include <Components/Npc/Npc.hpp>
#include <Components/Npc/Target.hpp>
#include <Components/Npc/Wisp.hpp>
#include <Components/Position.hpp>
#include <Factory/NpcFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/Threats/WispSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Npc.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>

#include <spdlog/spdlog.h>

namespace Game::Sys
{

WispSystem::WispSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank )
{
  SPDLOG_DEBUG( "WispSystem initialized" );
  m_wisp_target_reset_clock.reset();
}

void WispSystem::update( sf::Time dt )
{
  static constexpr float kPathfindingInterval = 0.10f;
  m_pathfinding_timer += dt;
  if ( m_pathfinding_timer.asSeconds() >= kPathfindingInterval )
  {
    if ( auto navmesh = m_open_navmesh.lock() )
    {
      for ( auto [target_entt, npc_target_cmp, npc_target_pos_cmp] : reg().view<Cmp::Npc::Target, Cmp::Position>().each() )
      {
        if ( not reg().valid( npc_target_cmp.id ) ) continue;
        bool target_in_spawn = Utils::Player::is_in_spawn( reg(), npc_target_pos_cmp );
        auto result = Utils::Npc::pathfind_toward( reg(), *navmesh, npc_target_pos_cmp, npc_target_cmp.id, target_in_spawn, false, true );
        if ( result == Utils::Npc::PathfindResult::NoPath )
        {
          if ( not m_wisp_target_reset_clock.isRunning() ) { m_wisp_target_reset_clock.restart(); }
          reset_wisp_target( npc_target_cmp.id );
        }
      }
    }

    m_pathfinding_timer = sf::Time::Zero;
  }
}

void WispSystem::spawn_wisp()
{
  // allow only max number of wisp
  const static int MAX_WISP_COUNT = 1;
  int wisp_count = 0;
  for ( auto [npc_entt, npc_cmp] : reg().view<Cmp::Npc::NPC>().each() )
  {
    if ( reg().any_of<Cmp::Npc::Wisp>( npc_entt ) ) wisp_count++;
  }
  if ( wisp_count >= MAX_WISP_COUNT ) return;

  // find a random start position in the game area and create a new npc at that position
  auto [spawn_entt, spawn_pos] = Utils::Rnd::get_random_position( reg(), {}, {} );
  auto npc_entt = Factory::Npc::create_npc( reg(), spawn_entt, "npc.wisp" );
  SPDLOG_DEBUG( "Created wisp npc {}", static_cast<uint32_t>( npc_entt ) );

  // find a random target position in the game area that is distant from the spawn_pos
  static constexpr float kMinTargetDistance = 100.f;
  auto [target_entt, target_pos] = Utils::Rnd::get_random_position( reg(), {}, {} );
  while ( Utils::Maths::getEuclideanDistance( target_pos.position, spawn_pos.position ) < kMinTargetDistance )
  {
    auto [new_target_entt, new_target_pos] = Utils::Rnd::get_random_position( reg(), {}, {} );
    target_entt = new_target_entt;
    target_pos = new_target_pos;
  }
  reg().emplace_or_replace<Cmp::Npc::Target>( target_entt, npc_entt );

  // register the wisp in the open navmesh so A* can find a valid start cell
  if ( auto open_navmesh = m_open_navmesh.lock() )
  {
    open_navmesh->insert( npc_entt, spawn_pos );
    open_navmesh->insert( target_entt, target_pos );
  }

  SPDLOG_INFO( "Spawned wisp {} at {},{}. Target is {},{}", static_cast<uint32_t>( npc_entt ), spawn_pos.x(), spawn_pos.y(), target_pos.x(),
               target_pos.y() );
}

void WispSystem::reset_wisp_target( entt::entity wisp_entt )
{

  const static float kWispTargetResetTimeout = 10.f;
  if ( m_wisp_target_reset_clock.getElapsedTime().asSeconds() < kWispTargetResetTimeout ) return;

  for ( auto [old_target_entt, old_npc_target_cmp, old_npc_target_pos_cmp] : reg().view<Cmp::Npc::Target, Cmp::Position>().each() )
  {
    if ( old_npc_target_cmp.id != wisp_entt ) continue;

    // find a random target position in the game area that is distant from the spawn_pos
    static constexpr float kMinTargetDistance = 100.f;
    auto [new_target_entt, new_target_pos] = Utils::Rnd::get_random_position( reg(), {}, {} );
    while ( Utils::Maths::getEuclideanDistance( new_target_pos.position, old_npc_target_pos_cmp.position ) < kMinTargetDistance )
    {
      auto [retry_target_entt, retry_target_pos] = Utils::Rnd::get_random_position( reg(), {}, {} );
      new_target_entt = retry_target_entt;
      new_target_pos = retry_target_pos;
    }
    reg().remove<Cmp::Npc::Target>( old_target_entt ); // remove from old target first
    reg().emplace_or_replace<Cmp::Npc::Target>( new_target_entt, wisp_entt );

    SPDLOG_INFO( "Reset wisp {} target to {},{}", static_cast<uint32_t>( wisp_entt ), new_target_pos.x(), new_target_pos.y() );
    m_wisp_target_reset_clock.reset();
    break; // only one target per wisp
  }
}

} // namespace Game::Sys
