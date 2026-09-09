#ifndef SRC_SYSTEMS_PROCGEN_DLASYSTEM_HPP__
#define SRC_SYSTEMS_PROCGEN_DLASYSTEM_HPP__

#include <Systems/BaseSystem.hpp>

namespace Game::Sys::ProcGen
{

//! @brief Diffusion-limited aggregation
class DLASystem : public BaseSystem
{
public:
  //! @brief Shape used to bias/constrain where DLA particles are spawned from.
  enum class SpawnShape {
    //! @brief Spawn particles from a circular boundary
    Circle,
    //! @brief Spawn particles from an elliptical boundary
    Ellipse
  };

  //! @brief Construct a new DLASystem object
  DLASystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
      : BaseSystem( reg, window, sprite_factory, sound_bank )
  {
  }

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief DLA here works by removing obstacles (carving space) rather than adding them.
  //! @details Carves the seed cell, then repeatedly spawns a particle at a random grid cell and
  //!          lets it random-walk until it lands adjacent to an already-carved cell, at which point
  //!          it too is carved - growing an organic, branching open area outward from the seed.
  //! @param scene_size World bounds of the scene the particles may walk within
  //! @param seed_pos World position of the initial carved (seed) cell that particles aggregate onto
  //! @param particle_limit Number of particles to spawn and walk to aggregation
  //! @param levelgen_spatialgrid Spatial grid of level-gen obstacles to read/update
  void iterate( sf::FloatRect scene_size, const sf::Vector2f seed_pos, uint16_t particle_limit, PathFinding::SpatialHashGrid &levelgen_spatialgrid,
                PathFinding::SpatialHashGrid &reserved_sm );
};

} // namespace Game::Sys::ProcGen

#endif // SRC_SYSTEMS_PROCGEN_DLASYSTEM_HPP__
