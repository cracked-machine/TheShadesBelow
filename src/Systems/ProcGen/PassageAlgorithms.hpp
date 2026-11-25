#ifndef SRC_SYSTEMS_PROCGEN_PASSAGEALGORITHMS_HPP__
#define SRC_SYSTEMS_PROCGEN_PASSAGEALGORITHMS_HPP__

#include <Components/Crypt/PassageBlock.hpp>
#include <Components/Crypt/PassageDoor.hpp>

#include <optional>

namespace Game::Sys::ProcGen
{
//! @brief Whether a newly-carved passage may overlap an already-carved passage block.
enum class AllowDuplicatePassages {
  //! @brief Allow the passage to overlap existing passage blocks
  YES,
  //! @brief Disallow the passage from overlapping existing passage blocks
  NO
};

//! @brief Whether a target room should only receive a single incoming passage.
enum class OnePassagePerTargetRoom {
  //! @brief Only carve one passage to the target room
  YES,
  //! @brief Allow multiple passages to the target room
  NO
};

//! @brief Which passage-carving algorithm to use.
enum class WalkingType {
  //! @brief Random, wandering "drunken walk" carving, biased towards the target
  DRUNK,
  //! @brief Straight horizontal-then-vertical (or vice versa) "dog leg" carving
  DOGLEG
};

//! @brief Whether to restrict a passage search to already-cached passage data only.
enum class CachedOnly {
  //! @brief Only consider cached passage data
  TRUE,
  //! @brief Also consider uncached/live passage data
  FALSE
};

//! @brief Min-heap of (distance-from-start, room entity) pairs, ordered nearest-first, used to
//!        pick candidate target rooms for passage carving.
using MidPointDistanceQueue = std::priority_queue<std::pair<float, entt::entity>, std::vector<std::pair<float, entt::entity>>,
                                                  std::greater<std::pair<float, entt::entity>>>;

//! @brief Algorithms for carving passage blocks between crypt rooms: placing individual passage
//!        blocks and walking/plotting full passages between a starting door and a target room.
class PassageAlogirthms
{
public:
  //! @brief Construct a new Passage Alogirthms object
  PassageAlogirthms() = default;

  //! @brief Place a passage block at the specified position
  //! @param reg Entity-component registry to query existing passage blocks/walls from
  //! @param x The x-coordinate of the position
  //! @param y The y-coordinate of the position
  //! @param duplicates_policy Whether to allow duplicate passages blocks
  //! @param skip_wall_check If true, skip the wall-collision check
  //! @return std::optional<Cmp::Crypt::PassageBlock> The placed block, or std::nullopt if blocked by a duplicate or a wall
  std::optional<Cmp::Crypt::PassageBlock> place_passage_block( entt::registry &reg, float x, float y,
                                                             AllowDuplicatePassages duplicates_policy = AllowDuplicatePassages::NO,
                                                             bool skip_wall_check = false );

  //! @brief Create a random, wandering "drunken walk" passage of blocks from the start door towards
  //!        the end bounds, biased to move towards the target while avoiding walls, rooms and other passages.
  //! @param reg Entity-component registry to query/carve passage blocks in
  //! @param start The starting position and direction for the passage
  //! @param end_bounds The bounds of the end point for the passage
  //! @param map_size_pixel Size of the map in pixels, used to keep the walk within bounds
  //! @param exclude_entts Entities to exclude from the search, e.g. the occupied room
  //! @param duplicates_policy Whether to allow duplicate passages blocks
  //! @return true if the passage was created successfully
  //! @return false if the passage could not be created
  std::vector<Cmp::Crypt::PassageBlock> create_drunken_walk( entt::registry &reg, Cmp::Crypt::PassageDoor start, sf::FloatRect end_bounds,
                                                           sf::Vector2f map_size_pixel, const std::set<entt::entity> &exclude_entts,
                                                           AllowDuplicatePassages duplicates_policy = AllowDuplicatePassages::NO );

  //! @brief Create a Dog Leg Passage between start and end points
  //! @param reg Entity-component registry to query/carve passage blocks in
  //! @param start The starting position and direction for the passage
  //! @param end_bounds The bounds of the end point for the passage
  //! @param duplicates_policy Whether to allow duplicate passages blocks
  //! @return true if the passage was created successfully
  //! @return false if the passage could not be created
  std::vector<Cmp::Crypt::PassageBlock> create_dog_leg( entt::registry &reg, Cmp::Crypt::PassageDoor start, sf::FloatRect end_bounds,
                                                      AllowDuplicatePassages duplicates_policy = AllowDuplicatePassages::NO );

  //! @brief Reset the current passage ID counter back to zero.
  void reset() { m_current_passage_id = 0; }

  //! @brief Get the current passage ID.
  //! @return unsigned int
  [[nodiscard]] unsigned int get_current_passage_id() const { return m_current_passage_id; }

  //! @brief Increment the current passage ID, starting a new logical passage.
  void increment_passage_id() { m_current_passage_id++; }

  //! @brief Precalculate and cache all wall obstacle bounds so passage carving can check for
  //!        wall collisions without repeatedly querying the registry.
  //! @param reg Entity-component registry to query wall obstacles from
  void cache_wall_components( entt::registry &reg );

private:
  //! @brief Current passage ID for new passages
  unsigned int m_current_passage_id{ 0 };

  //! @brief Precalculated list of wall obstacles (created during scene init)
  std::vector<sf::FloatRect> m_cached_wall_components;
};

} // namespace Game::Sys::ProcGen

#endif // SRC_SYSTEMS_PROCGEN_PASSAGEALGORITHMS_HPP__
