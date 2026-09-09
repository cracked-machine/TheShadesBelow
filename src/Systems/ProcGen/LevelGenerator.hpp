#ifndef SRC_SYSTEMS_PROCGEN_LEVELGENERATOR_HPP__
#define SRC_SYSTEMS_PROCGEN_LEVELGENERATOR_HPP__

#include <PathFinding/SmartPointers.hpp>
#include <Sprites/SpriteMetaType.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Window.hpp>

#include <cstddef>
#include <functional>

// clang-format off
namespace Game::Cmp { class Position; class RectBounds; }
namespace sf { class RenderWindow; }
namespace Game::PathFinding { class SpatialHashGrid; } 
namespace Game::Sprites { class SpriteSheet; class SpriteFactory; }
namespace Game::Scene { class SceneData; }
// clang-format on

namespace Game::Sys::ProcGen
{

//! @brief Builds a scene's game area from SceneData and drives procedural generation of obstacles,
//!        decorations, and multiblocks, owning the spatial grids used to track obstacle/void/decor
//!        placement during level generation and the cellular-automata pass.
class LevelGenerator : public BaseSystem
{
public:
  //! @brief Construct a new Level Generator object
  LevelGenerator( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

  //! @brief Destroy the Level Generator object
  ~LevelGenerator() = default;

  //! @brief Call this to make sure the level data is reset before regenerating a new scene
  //! @param reserved_sm Scene-owned grid of positions reserved from procgen/algorithmic changes,
  //!        populated incrementally as this generation pass places entities.
  void init( const PathFinding::SpatialHashGridSharedPtr &reserved_sm );

  //! @brief Generate game area using data from the SceneData object.
  //! @param scene_data The deserialized scene data to build the game area from
  void build_scene_from_data( const Scene::SceneData &scene_data );

  //! @brief Create obstacle components without sprites for initial proc gen
  //! @param init_chance Probability (0-1) that any given eligible position becomes an obstacle
  //! @param reserved_sm Positions excluded from obstacle placement
  void add_graveyard_exterior_obstacles( float init_chance );

  //! @brief create "sprite.graveyard.wall.int.main" and "sprite.graveyard.wall.int.cap" sprites for the graveyard obstacles.
  void decorate_graveyard_exterior_obstacles();

  //! @brief Create obstacle components without sprites for initial proc gen
  //! @param init_chance Probability (0-1) that any given eligible position becomes an obstacle
  //! @param reserved_sm Positions excluded from obstacle placement
  void add_ruin_interior_obstacles( float init_chance );

  //! @brief Create "sprite.crypt.wall.int" sprites for the graveyard obstacles.
  void decorate_ruin_interior_obstacles();

  //! @brief Place a handful of decorative rune-marking entities at random unreserved positions.
  void add_ruin_rune_markers();

  //! @brief Scatter decorative cobweb entities across the lower floor, skipping tiles that would
  //!        collide with stairs, other cobwebs, exits, or the edge of the scene.
  //! @param num_cobwebs Number of cobwebs to attempt to place.
  //! @param scene_dimensions World bounds of the scene to scatter cobwebs within.
  void add_lowerfloor_cobwebs( int num_cobwebs, sf::FloatRect scene_dimensions );

  //! @brief create multiblock sprites (i.e. graves, altars, crypts) for the graveyard scene.
  void gen_graveyard_exterior_multiblocks();

  //! @brief Find a valid spawn location for a large obstacle given a seed.
  //! @param ms Sprite sheet of the object being placed, used to size its collision hitbox
  //! @param seed Seed for the random position search
  //! @return std::pair<entt::entity, Cmp::Position>
  std::pair<entt::entity, Cmp::Position> find_spawn_location( const Sprites::SpriteSheet &ms, unsigned long seed );

  //! @brief Generate a number of plant world items in the new game area.
  //! @param map_grid_size Size of the game area, in grid cells
  //! @param reserved_sm Positions excluded from plant placement
  //! @return std::vector<entt::entity>
  std::vector<entt::entity> gen_random_plants( sf::Vector2u map_grid_size );

  bool gen_plant( const std::string &plant_type, sf::Vector2f pos );

  //! @brief Scatter loot containers across the map at random unreserved positions.
  //! @param reg
  //! @param sprite_factory Used to pick sprite size/random sprite index for each container.
  //! @param map_grid_size Map size in grid cells; determines how many containers to place.
  //! @param reserved_sm Positions already reserved are skipped.
  //! @return The entities that were assigned loot containers.
  std::vector<entt::entity> gen_loot_containers( Sprites::SpriteFactory &sprite_factory, sf::Vector2u map_grid_size );

  //! @brief Scatter NPC spawn containers across the map at random unreserved positions.
  //! @param reg
  //! @param sprite_factory Used to pick a random container sprite type/index.
  //! @param map_grid_size Map size in grid cells; determines how many containers to place.
  //! @param reserved_sm Positions already reserved are skipped.
  //! @return The entities that were assigned NPC containers.
  std::vector<entt::entity> gen_npc_containers( Sprites::SpriteFactory &sprite_factory, sf::Vector2u map_grid_size );

  //! @brief Get the obstacle sm object
  //! @return PathFinding::SpatialHashGrid&
  PathFinding::SpatialHashGrid &get_obstacle_sm();

  //! @brief Get the void sm object
  //! @return PathFinding::SpatialHashGrid&
  PathFinding::SpatialHashGrid &get_void_sm();

  //! @brief Get the non obstacle sm object
  //! @return PathFinding::SpatialHashGrid&
  PathFinding::SpatialHashGrid &get_non_obstacle_sm();

  //! @brief Get the reserved sm object
  //! @return PathFinding::SpatialHashGrid&
  PathFinding::SpatialHashGrid &get_reserved_sm();

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Roll `init_chance` for `entity`/`pos_cmp` and, if it succeeds, register it as an obstacle in
  //!        both `m_obstacle_sm` and `reserved_sm`.
  //! @param pass_navmesh_to_factory Whether Factory::Obstacle::add_obstacle() should itself check `reserved_sm`
  //!        (graveyard case) or whether the caller has already guaranteed the position is free (ruin case).
  void try_place_obstacle( entt::entity entity, const Cmp::Position &pos_cmp, float init_chance, bool pass_navmesh_to_factory );

  //! @brief Shared obstacle-decoration pass: sprites the obstacle entity and its paired "cap" entity, tagging
  //!        both with a shared UUID.
  //! @param pick_index Returns the sprite-sheet frame index to use for the obstacle and its cap.
  //! @param cap_y_offset Added to the obstacle's y-position when computing the cap's z-order.
  //! @param moveable Whether to tag the obstacle entity with Cmp::Moveable.
  void decorate_obstacles( const Sprites::SpriteSheet &ss_main, const Sprites::SpriteSheet &ss_cap, const std::function<std::size_t()> &pick_index,
                           float cap_y_offset, bool moveable );

  //! @brief Spawn `count` multiblocks of the given type at random valid positions found via find_spawn_location().
  template <typename MULTIBLOCK, typename MBSEGMENT>
  void spawn_multiblocks( std::size_t count, const Sprites::SpriteSheet &ss, bool log = false );

  //! @brief Spatial map for finding obstacles during level gen / cell automata algorithm
  PathFinding::SpatialHashGridUniquePtr m_obstacle_sm;

  //! @brief Spatial map for marking void areas that are not part of the game area.
  PathFinding::SpatialHashGridUniquePtr m_void_sm;

  //! @brief Spatial map for decorative, non-obstacle entities placed during level gen (e.g. runes, cobwebs).
  PathFinding::SpatialHashGridUniquePtr m_non_obstacle_sm;

  //! @brief Scene-owned spatial map for reserving positions during procedural generation, shared
  //!        with gameplay Systems for the lifetime of the scene.
  PathFinding::SpatialHashGridSharedPtr m_reserved_sm;
};

} // namespace Game::Sys::ProcGen

#endif // SRC_SYSTEMS_PROCGEN_LEVELGENERATOR_HPP__
