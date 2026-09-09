#ifndef SRC_SYSTEMS_PROCGEN_CELLAUTOMATASYSTEM_HPP__
#define SRC_SYSTEMS_PROCGEN_CELLAUTOMATASYSTEM_HPP__

#include <Systems/BaseSystem.hpp>
#include <Systems/ProcGen/LevelGenerator.hpp>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entity/fwd.hpp>

// clang-format off
namespace Components { class Neighbours; class Obstacle; class Position; class Random; }
namespace PathFinding { class SpatialHashGrid; }
// clang-format on

namespace Game::Sys::ProcGen
{

//! @brief Applies a cellular-automata pass over the level's position grid, turning obstacles on/off
//!        based on neighbour counts to smooth out proc-gen noise into organic-looking cave/wall shapes.
class CellAutomataSystem : public BaseSystem
{
public:
  //! @brief Construct a new Cell Automata System object
  CellAutomataSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
      : BaseSystem( reg, window, sprite_factory, sound_bank )
  {
  }

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief Run the cellular-automata birth/survival rule over every non-reserved position for the given
  //!        number of iterations, adding or removing obstacles depending on live-neighbour counts.
  //! @param levelgen_spatialgrid Spatial grid of level-gen obstacles to read/update
  //! @param reserved_sm Positions excluded from the automata pass (e.g. spawn/reserved areas)
  void iterate( PathFinding::SpatialHashGrid &levelgen_spatialgrid, PathFinding::SpatialHashGrid reserved_sm );
};

} // namespace Game::Sys::ProcGen

#endif // SRC_SYSTEMS_PROCGEN_CELLAUTOMATASYSTEM_HPP__
