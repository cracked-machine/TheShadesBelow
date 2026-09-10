#include <Components/Moveable.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Persistent/GraveyardProcGenBirthThreshold.hpp>
#include <Components/Persistent/GraveyardProcGenMaxIterations.hpp>
#include <Components/Persistent/GraveyardProcGenSurvivalThreshold.hpp>
#include <Components/Player/Character.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/ProcGen/CellAutomataSystem.hpp>

#include <ranges>
#include <spdlog/spdlog.h>

namespace Game::Sys::ProcGen
{

void CellAutomataSystem::iterate( PathFinding::SpatialHashGrid &levelgen_spatialgrid, PathFinding::SpatialHashGrid &reserved_sm )
{
  auto iterations = Sys::PersistSystem::get<Cmp::Persist::GraveyardProcGenMaxIterations>( m_reg ).get_value();
  auto birth_threshold = Sys::PersistSystem::get<Cmp::Persist::GraveyardProcGenBirthThreshold>( m_reg ).get_value();
  auto survival_threshold = Sys::PersistSystem::get<Cmp::Persist::GraveyardProcGenSurvivalThreshold>( m_reg ).get_value();
  std::string iteration_timings;

  sf::Clock iteration_timer;
  for ( unsigned int i = 0; i < iterations; i++ )
  {

    SPDLOG_DEBUG( "NAVMESH: {}", levelgen_spatialgrid.size() );
    for ( auto [pos_entt, pos_cmp] : reg().view<Cmp::Position>().each() )
    {
      // 1. If the cell is dead and has at least birth-threshold alive neighbors, it becomes alive.
      // 2. If the cell is alive and has at least survival-threshold alive neighbors, it stays alive.
      // 3. Otherwise, the cell is dead.
      const bool is_alive = reg().any_of<Cmp::Obstacle>( pos_entt );

      // Positions reserved before this pass started (walls, structures, spawn, etc.) that are not
      // themselves obstacles are permanently off-limits - skip them entirely. Positions that became
      // obstacles during this same proc-gen pass are also inserted into reserved_sm (see below), but
      // must stay eligible for survival/death checks like any other obstacle, so only skip when not
      // already alive.
      if ( not is_alive && not reserved_sm.at( pos_cmp ).empty() ) continue;

      std::vector<entt::entity> neighbour_list = levelgen_spatialgrid.neighbours( pos_cmp );
      SPDLOG_DEBUG( "#{} at {},{} has {} nieghbours", static_cast<uint32_t>( pos_entt ), pos_cmp.x(), pos_cmp.y(), neighbour_list.size() );
      const bool should_be_alive = ( not is_alive && neighbour_list.size() >= birth_threshold ) ||
                                   ( is_alive && neighbour_list.size() >= survival_threshold );

      if ( should_be_alive )
      {
        if ( reserved_sm.at( pos_cmp ).empty() )
        {
          // Bypass add_obstacle to avoid its O(n) reserved-position scan in this hot loop;
          // reserved_grid.at() above is already the correct O(1) guard.
          // reg().emplace_or_replace<Cmp::Obstacle>( pos_entt );
          // reg().emplace_or_replace<Cmp::Npc::NoPathFinding>( pos_entt );
          Factory::Obstacle::add_obstacle( reg(), pos_entt );
          reserved_sm.insert( pos_entt, pos_cmp );
        }
      }
      else
      {
        // make sure we dont delete the player character by accident
        if ( not reg().any_of<Cmp::Player::Character>( pos_entt ) )
        {
          Factory::Obstacle::remove_obstacle( reg(), pos_entt );
          reserved_sm.remove( pos_entt, pos_cmp );
        }
      }
    }
    iteration_timings += " " + std::to_string( iteration_timer.restart().asMilliseconds() );
  }
  iteration_timings += " ";
  SPDLOG_INFO( "Iterations (ms): [{}]", iteration_timings );
  SPDLOG_DEBUG( "Total Iterations: {}", iterations );
}

} // namespace Game::Sys::ProcGen