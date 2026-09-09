#include <Components/Random.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <Systems/ProcGen/DLASystem.hpp>
#include <Utils/Cardinal.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Utils.hpp>

namespace Game::Sys::ProcGen
{

void DLASystem::iterate( sf::FloatRect scene_size, const sf::Vector2f seed_pos, uint16_t particle_limit,
                         PathFinding::SpatialHashGrid &levelgen_spatialgrid, PathFinding::SpatialHashGrid &reserved_sm )
{
  // Carve the seed at center
  auto center_entities = levelgen_spatialgrid.at( Cmp::Position( seed_pos, Constants::kGridSizePxF ) );
  for ( auto &entt : center_entities )
  {
    Factory::Obstacle::remove_obstacle( reg(), entt );
    const Cmp::Position remove_pos( seed_pos, Constants::kGridSizePxF );
    levelgen_spatialgrid.remove( entt, remove_pos );
    reserved_sm.remove( entt, remove_pos );
  }
  SPDLOG_INFO( "Seed particle position: {},{}", seed_pos.x, seed_pos.y );

  Cmp::RandomInt angle_picker( 0, 359 );
  Cmp::RandomInt dir_picker( 0, 3 );

  // Random position pickers across the full scene interior (inset 1 cell)
  Cmp::RandomInt rand_col( 1, static_cast<int>( ( scene_size.size.x / Constants::kGridSizePxF.x ) ) - 2 );
  Cmp::RandomInt rand_row( 1, static_cast<int>( ( scene_size.size.y / Constants::kGridSizePxF.y ) ) - 2 );

  int particle_count = 0;
  while ( particle_count < particle_limit )
  {
    // Spawn at a uniformly random grid cell inside the scene
    sf::Vector2f spawn_pos = scene_size.position +
                             sf::Vector2f( rand_col.gen() * Constants::kGridSizePxF.x, rand_row.gen() * Constants::kGridSizePxF.y );

    Cmp::Position particle( Utils::snap_to_grid( spawn_pos ), Constants::kGridSizePxF );

    bool stuck = false;
    int steps = 0;
    const int max_steps = 10000; // prevent truly runaway walkers

    while ( not stuck and steps < max_steps )
    {
      ++steps;

      if ( not scene_size.contains( particle.position ) ) break;

      // Check if adjacent to a carved (empty) cell
      bool adjacent_to_aggregate = false;
      for ( auto c : Utils::Cardinal() )
      {
        sf::Vector2f neighbour_world = Utils::snap_to_grid( particle.position + c.vector().componentWiseMul( Constants::kGridSizePxF ) );
        if ( not scene_size.contains( neighbour_world ) ) continue;
        Cmp::Position neighbour( neighbour_world, Constants::kGridSizePxF );
        if ( levelgen_spatialgrid.at( neighbour ).empty() )
        {
          adjacent_to_aggregate = true;
          break;
        }
      }

      if ( adjacent_to_aggregate )
      {
        auto entities_here = levelgen_spatialgrid.at( particle );
        for ( auto &entt : entities_here )
        {
          Factory::Obstacle::remove_obstacle( reg(), entt );
          levelgen_spatialgrid.remove( entt, particle );
          reserved_sm.remove( entt, particle );
        }
        stuck = true;
        ++particle_count;
        SPDLOG_DEBUG( "Carved {},{} particle {}/{}", particle.x(), particle.y(), particle_count, particle_limit );
      }
      else
      {
        Utils::Cardinal random_cardinal( static_cast<Utils::Cardinal::Value>( dir_picker.gen() ) );
        sf::Vector2f new_pos = particle.position + random_cardinal.vector().componentWiseMul( Constants::kGridSizePxF );
        particle = Cmp::Position( Utils::snap_to_grid( new_pos ), Constants::kGridSizePxF );
      }
    }
  }
}

} // namespace Game::Sys::ProcGen