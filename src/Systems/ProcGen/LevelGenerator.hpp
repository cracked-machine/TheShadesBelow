#ifndef SRC_SYSTEMS_PROCGEN_LEVELGENERATOR_HPP__
#define SRC_SYSTEMS_PROCGEN_LEVELGENERATOR_HPP__

#include <PathFinding/SmartPointers.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Window.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <vector>

// clang-format off
namespace Game::Cmp { class Position; class RectBounds; }
namespace sf { class RenderWindow; }
namespace Game::PathFinding { class SpatialHashGrid; } 
namespace Game::Sprites { class SpriteSheet; }
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
  LevelGenerator( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Destroy the Level Generator object
  ~LevelGenerator() = default;

  //! @brief Call this to make sure the level data is reset before regenerating a new scene
  //! @param reserved_sm Scene-owned grid of static structures, populated incrementally as this generation pass places them.
  //! @param item_sm Scene-owned grid of ground world items (see Scene::m_item_sm); generation never places over an item.
  void init( const PathFinding::SpatialHashGridSharedPtr &reserved_sm, const PathFinding::SpatialHashGridSharedPtr &item_sm = nullptr );

  //! @brief Set the scene's world-tile lookup grid (see Scene::m_world_tile_sm). It's built after
  //!        build_scene_from_data(), so it's set separately from init().
  //! @param world_tile_sm
  void set_world_tile_sm( const PathFinding::SpatialHashGridSharedPtr &world_tile_sm ) { m_world_tile_sm = world_tile_sm; }

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

  //! @brief create multiblock sprites (crypt, healing building, ruin and graves) for the graveyard scene.
  //!        The crypts, and the healing building / ruin if present, are placed at the given positions (the scene's
  //!        'poi' markers).
  //!        Graves are then laid out in clusters, as many as fit in each map quadrant: each cluster is an N x N grid
  //!        (Cmp::Persist::GraveClusterGridSize) of 2x2-cell grave slots with kGraveClusterInternalPadding cells between
  //!        slots, and Cmp::Persist::GraveClusterExternalPadding cells between clusters. A grave is omitted when its
  //!        footprint is not open levelgen ground or is already taken (walls, paths, buildings, ...). The clusters'
  //!        internal padding cells are kept for gen_random_plants(). One random grave per quadrant is tagged
  //!        Cmp::Grave::KeyGrave, so it drops a crypt key.
  //! @param scene_data Scene data used to split the map into quadrants and find open levelgen tiles
  //! @param crypt_positions Top-left pixel positions of the crypts.
  //! @param healing_pos Top-left pixel position of the healing spring / barber surgeon building, or nullopt for none.
  //! @param ruin_pos Top-left pixel position of the ruin, or nullopt for none.
  void gen_graveyard_exterior_multiblocks( const Scene::SceneData &scene_data, std::span<const sf::Vector2f> crypt_positions,
                                           std::optional<sf::Vector2f> healing_pos, std::optional<sf::Vector2f> ruin_pos );

  //! @brief Find a valid spawn location for a large obstacle given a seed.
  //! @param ms Sprite sheet of the object being placed, used to size its collision hitbox
  //! @param seed Seed for the random position search
  //! @return std::pair<entt::entity, Cmp::Position>
  std::pair<entt::entity, Cmp::Position> find_spawn_location( const Sprites::SpriteSheet &ms, unsigned long seed );

  //! @brief Generate plant world items in the internal padding of the grave clusters, via Events::CreatePlantEvent (see
  //!        PlantSystem). Each plant's items.json "spawn_count_per_quadrant" is placed in every map quadrant (see
  //!        SceneData::quadrant_of()), as far as that quadrant's cluster padding allows. Call after
  //!        gen_graveyard_exterior_multiblocks().
  //! @param scene_data Scene data used to split the map into quadrants
  //! @return std::vector<entt::entity>
  std::vector<entt::entity> gen_random_plants( const Scene::SceneData &scene_data );

  //! @brief Grow grass (Cmp::Plant::kGrassItem, via Events::CreatePlantEvent) on the free open-ground cells of every map quadrant,
  //!        each with a Cmp::Persist::GrassSpawnChance percent chance. Call after the obstacles are decorated and before
  //!        discard_claims(), so it only fills what graves, plants, containers and obstacles left free.
  //! @param scene_data Scene data used to find the open levelgen tiles and split the map into quadrants
  void gen_grass( const Scene::SceneData &scene_data );

  //! @brief Scatter loot containers across the map at random unreserved positions.
  //! @param reg
  //! @param map_grid_size Map size in grid cells; determines how many containers to place.
  //! @param reserved_sm Positions already reserved are skipped.
  //! @return The entities that were assigned loot containers.
  std::vector<entt::entity> gen_loot_containers( sf::Vector2u map_grid_size );

  //! @brief Place NPC spawn containers at random free open-ground cells inside or on the edge of the grave clusters
  //!        (m_grave_cluster_bounds expanded by one cell). If those cells run out, moves out one ring at a time around
  //!        every cluster until all containers are placed. Call after gen_graveyard_exterior_multiblocks().
  //! @param scene_data Scene data used for the map size, which determines how many containers to place, and the open tiles
  //! @return The entities that were assigned NPC containers.
  std::vector<entt::entity> gen_npc_containers( const Scene::SceneData &scene_data );

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

  //! @brief Get the level-generation claims grid (see m_claims_sm).
  //! @return PathFinding::SpatialHashGrid&
  PathFinding::SpatialHashGrid &get_claims_sm();

  //! @brief Whether a generation pass must skip this cell: a structure (reserved), a world item, or a placement claimed
  //!        earlier in this pass.
  //! @param pos The cell to test.
  //! @return bool true if the cell is taken.
  bool is_cell_taken( const Cmp::Position &pos ) const;

  //! @brief Rect variant of is_cell_taken() for multi-cell footprints.
  //! @param bounds World-space rectangle to test.
  //! @return bool true if any cell under `bounds` is taken.
  bool is_area_taken( const sf::FloatRect &bounds ) const;

  //! @brief End of level generation: the claimed obstacles/caps/containers/NPC spawn cells don't need protecting any more,
  //!        so drop every claim.
  void discard_claims();

private:
  //! @brief Grid cells per side of a grave cluster slot: fits both the 2x2 large and the 1x2 small graves.
  static constexpr int kGraveSlotCells{ 2 };
  //! @brief Grid cells between neighbouring grave slots inside a grave cluster.
  static constexpr int kGraveClusterInternalPadding{ 1 };

  //! @brief Whether a multiblock hitbox collides with walls, graves, buildings, the spawn area or the player.
  //!        Does not check the reserved/claims/item grids, see is_area_taken() / is_cell_taken().
  //! @param hitbox World-space hitbox to test.
  //! @return bool true on any collision.
  bool has_structure_collision( const Cmp::RectBounds &hitbox );

  //! @brief Roll `init_chance` for `entity`/`pos_cmp` and, if it succeeds, register it as an obstacle in
  //!        both `m_obstacle_sm` and `m_claims_sm`. Callers must already have checked is_cell_taken().
  void try_place_obstacle( entt::entity entity, const Cmp::Position &pos_cmp, float init_chance );

  //! @brief Shared obstacle-decoration pass: sprites the obstacle entity and its paired "cap" entity, tagging
  //!        both with a shared UUID.
  //! @param pick_index Returns the sprite-sheet frame index to use for the obstacle and its cap.
  //! @param cap_y_offset Added to the obstacle's y-position when computing the cap's z-order.
  //! @param moveable Whether to tag the obstacle entity with Cmp::Moveable.
  void decorate_obstacles( const Sprites::SpriteSheet &ss_main, const Sprites::SpriteSheet &ss_cap, const std::function<std::size_t()> &pick_index,
                           float cap_y_offset, bool moveable );

  //! @brief Spawn a multiblock of the given type at a fixed position and reserve it.
  //! @param pos Top-left pixel position of the multiblock.
  //! @param ss Sprite sheet of the multiblock.
  //! @return The multiblock entity.
  template <typename MULTIBLOCK, typename MBSEGMENT>
  entt::entity spawn_multiblock_at( sf::Vector2f pos, const Sprites::SpriteSheet &ss );

  //! @brief Spatial map for finding obstacles during level gen / cell automata algorithm
  PathFinding::SpatialHashGridUniquePtr m_obstacle_sm;

  //! @brief Spatial map for marking void areas that are not part of the game area.
  PathFinding::SpatialHashGridUniquePtr m_void_sm;

  //! @brief Spatial map for decorative, non-obstacle entities placed during level gen (e.g. runes, cobwebs).
  PathFinding::SpatialHashGridUniquePtr m_non_obstacle_sm;

  //! @brief Scene-owned spatial map for reserving positions during procedural generation, shared
  //!        with gameplay Systems for the lifetime of the scene.
  PathFinding::SpatialHashGridSharedPtr m_reserved_sm;

  //! @brief Cells claimed by placements made during this generation pass (obstacles, obstacle caps, loot containers,
  //!        developer-layer NPC spawn cells), so later passes don't overlap them. Not structures: discard_claims() ends
  //!        their life once generation is done.
  PathFinding::SpatialHashGrid m_claims_sm;

  //! @brief Scene-owned ground world items, so generation passes don't place over them.
  PathFinding::SpatialHashGridWeakPtr m_item_sm;

  //! @brief Scene-owned world-tile lookup, passed to the multiblock factory to avoid full Cmp::Position scans.
  PathFinding::SpatialHashGridWeakPtr m_world_tile_sm;

  //! @brief Pixel positions of every grave cluster's internal padding cells, by map quadrant (see SceneData::quadrant_of()).
  //!        Filled by gen_graveyard_exterior_multiblocks(), used as the plant candidates by gen_random_plants().
  std::array<std::vector<sf::Vector2f>, 4> m_grave_cluster_padding;

  //! @brief Bounding box of the graves actually placed in each grave cluster, in grid cells. Clusters with no graves are
  //!        left out. Filled by gen_graveyard_exterior_multiblocks(), used by gen_npc_containers().
  std::vector<sf::IntRect> m_grave_cluster_bounds;
};

} // namespace Game::Sys::ProcGen

#endif // SRC_SYSTEMS_PROCGEN_LEVELGENERATOR_HPP__
