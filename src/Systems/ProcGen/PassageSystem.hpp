#ifndef SRC_SYSTEMS_PROCGEN_PASSAGESYSTEM_HPP__
#define SRC_SYSTEMS_PROCGEN_PASSAGESYSTEM_HPP__

#include <Components/Crypt/RoomClosed.hpp>
#include <Components/Crypt/RoomOpen.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <SceneControl/Scene.hpp>
#include <SceneControl/SmartPointers.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/ProcGen/PassageAlgorithms.hpp>

#include <entt/fwd.hpp>

// clang-format off
namespace Game::Cmp::Crypt { class PassageDoor; class PassageBlock; class RoomClosed; class RoomOpen; }
namespace Game::Events { class PassageEvent; }
namespace Game::PathFinding { class SpatialHashGrid; }
// clang-format on

namespace Game::Sys
{

//! @brief Carves and manages passage blocks that connect crypt rooms together, driven by
//!        Events::PassageEvent. Handles connecting the start/occupied/end rooms to nearby open
//!        or closed rooms, caching passage layouts per region, and opening/filling passages as
//!        the player explores the level.
class PassageSystem : public Game::Sys::BaseSystem
{
public:
  //! @brief Construct a new Passage System object
  PassageSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

  //! @brief Store the crypt scene data used to look up map size for passage carving.
  //! @param crypt_scene_data Weak-owning pointer to the crypt scene's deserialized scene data
  void init_scene_data( const Scene::SceneMapSharedPtr &crypt_scene_data ) { m_crypt_scene_data = crypt_scene_data; }

  //! @brief init the weak pointer for the reserved-positions grid, ahead of passage carving.
  //! @param reserved_sm Positions excluded from passage carving/decoration.
  void init_reserved_sm( const PathFinding::SpatialHashGridSharedPtr &reserved_sm ) { m_reserved_sm = reserved_sm; }

  //! @brief init the weak pointer for the spatial grid
  //! @param npc_navmesh Spatial grid used for NPC pathfinding
  void init_nav_mesh( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh );

  //! @brief Dispatches a Events::PassageEvent to the corresponding passage operation
  //!        (remove, open, connect rooms, cache connections, or add spike traps).
  //! @param event The passage event describing which operation to perform
  void on_passage_event( Events::PassageEvent &event );

  //! @brief Per-frame update; when a full-level connection pass has been requested it spawns
  //!        one cached passage region per call until all regions are placed.
  //! @param dt Time elapsed since the last update
  void update( sf::Time dt );

  //! @brief Empty out passage block areas, removing obstacles/chests so passages become walkable.
  void open_passages();

  //! @brief Create west, north, east passages for the start room via find_passage_target()
  //! @param start_room_entt The entity ID of the start room
  void connect_start_and_open_rooms_passages( entt::entity start_room_entt );

  //! @brief Create west, north, east and south passages for the occupied room via find_passage_target()
  void connect_occupied_and_open_room_passages();

  //! @brief Create west, north, east and south passages for all open rooms via find_passage_target()
  void cache_all_room_connections();

  //! @brief Connect the end room to the nearest reachable closed room via its south door, marking
  //!        that closed room as the BFS root for later room-connectivity checks.
  void connect_end_room_to_nearest_closed_room();

  //! @brief Create north passage for occupied room to the end room. Calls createDrunkenWalkPassage() directly.
  //! @param reg Entity-component registry to query/carve passage blocks in
  //! @param end_room_entt The entity ID of the end room
  //! @param map_size_pixel Size of the map in pixels, used to keep the walk within bounds
  void connect_occupied_and_end_room_passages( entt::registry &reg, entt::entity end_room_entt, sf::Vector2f map_size_pixel );

  //! @brief Add spike traps along each already-carved passage, one per passage ID.
  void add_spike_traps();

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Removes all Cmp::Crypt::PassageBlock entities
  void remove_all_passage_blocks();

  //! @brief Removes Cmp::Obstacles from Cmp::Crypt::PassageBlock areas
  void empty_open_passages();

  //! @brief Restores missing Cmp::Obstacles components to Cmp::Crypt::PassageBlock areas
  void fill_all_passages();

  //! @brief Removes any Cmp::Crypt::PassageBlock components added inside rooms
  //! @param include_closed_rooms If true, also tidy passage blocks found inside closed rooms
  void tidy_passage_blocks( bool include_closed_rooms = false );

  //! @brief Instantiate every passage block queued in m_uncached_passage_list as an entity.
  void create_uncached_passages();

  //! @brief Instantiate the passage blocks for the region at m_region_idx (from m_cached_passage_list)
  //!        as entities, advancing to the next region; once all regions are spawned it opens the
  //!        passages and adds spike traps.
  void create_cached_passages();

  //! @brief Build a priority queue of candidate target rooms of type ROOMTYPE within a search
  //!        quadrant, ordered nearest-first by distance from the starting door.
  //! @tparam ROOMTYPE The room component type to search for (e.g. Cmp::Crypt::RoomOpen)
  //! @param start_passage_door Door the passage will start from
  //! @param search_quadrant World bounds to restrict the room search to
  //! @param exclude_entts Room entities to exclude from the search
  //! @return ProcGen::MidPointDistanceQueue
  template <typename ROOMTYPE>
  ProcGen::MidPointDistanceQueue find_room_distances( Cmp::Crypt::PassageDoor &start_passage_door, const sf::FloatRect &search_quadrant,
                                                      std::set<entt::entity> exclude_entts );

  //! @brief Walk the distance-ordered room queue, attempting to carve a passage to each candidate
  //!        room in turn (nearest first) until one succeeds, and return the resulting passage blocks.
  //! @tparam ROOMTYPE The room component type of the candidate rooms in dist_pqueue
  //! @param start_passage_door Door the passage will start from
  //! @param dist_pqueue Nearest-first queue of candidate target rooms
  //! @param walktype Which passage-carving algorithm to use
  //! @param map_size_pixel Size of the map in pixels, used to keep the walk within bounds
  //! @param passage_limit Whether to stop after the first passage or allow multiple to the same target room
  //! @param duplicates_policy Whether to allow duplicate passage blocks
  //! @return std::vector<Cmp::Crypt::PassageBlock>
  template <typename ROOMTYPE>
  std::vector<Cmp::Crypt::PassageBlock>
  find_passages( Cmp::Crypt::PassageDoor &start_passage_door, ProcGen::MidPointDistanceQueue &dist_pqueue, ProcGen::WalkingType walktype,
                 sf::Vector2f map_size_pixel, ProcGen::OnePassagePerTargetRoom passage_limit, ProcGen::AllowDuplicatePassages duplicates_policy );

  //! @brief Used for NPC pathfinding
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;

  //! @brief Holds data about the Crypt scene levelgen and static multiblock objects
  Scene::SceneMapWeakPtr m_crypt_scene_data;

  //! @brief index for keeping track of the current `m_cached_passage_list` position.
  size_t m_region_idx{ 0 };

  //! @brief flag for enabling final dynamic scene update via the update function
  bool m_connect_all_rooms{ false };

  //! @brief Precalculated passage blocks for every room connection, bucketed by region, spawned
  //!        incrementally by create_cached_passages() during the final dynamic scene update.
  ProcGen::PassageCachedRegions<40> m_cached_passage_list;

  //! @brief The list of passage blocks for all paths during dynamic scene updates
  std::vector<Cmp::Crypt::PassageBlock> m_uncached_passage_list;

  //! @brief Owns the drunken-walk/dog-leg passage carving algorithms and the current passage ID.
  ProcGen::PassageAlogirthms m_passage_algos;

  //! @brief Spatial grid of all currently-instantiated passage block entities.
  PathFinding::SpatialHashGrid m_passage_block_grid;
};

// clang-format off
extern template std::vector<Cmp::Crypt::PassageBlock>
PassageSystem::find_passages<Cmp::Crypt::RoomOpen>( 
  Cmp::Crypt::PassageDoor &, 
  ProcGen::MidPointDistanceQueue &, 
  ProcGen::WalkingType, 
  sf::Vector2f,
  ProcGen::OnePassagePerTargetRoom, 
  ProcGen::AllowDuplicatePassages 
);

extern template std::vector<Cmp::Crypt::PassageBlock>
PassageSystem::find_passages<Cmp::Crypt::RoomClosed>( 
  Cmp::Crypt::PassageDoor &, 
  ProcGen::MidPointDistanceQueue &, 
  ProcGen::WalkingType, 
  sf::Vector2f,
  ProcGen::OnePassagePerTargetRoom, 
  ProcGen::AllowDuplicatePassages 
);

extern template ProcGen::MidPointDistanceQueue 
PassageSystem::find_room_distances<Cmp::Crypt::RoomOpen>( 
  Cmp::Crypt::PassageDoor &, 
  const sf::FloatRect &,
  std::set<entt::entity> 
);

extern template ProcGen::MidPointDistanceQueue
PassageSystem::find_room_distances<Cmp::Crypt::RoomClosed>( 
  Cmp::Crypt::PassageDoor &, 
  const sf::FloatRect &, 
  std::set<entt::entity>
);
// clang-format on

} // namespace Game::Sys

#endif // SRC_SYSTEMS_PROCGEN_PASSAGESYSTEM_HPP__
