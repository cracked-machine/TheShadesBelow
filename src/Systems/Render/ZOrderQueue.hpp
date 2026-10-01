#ifndef SRC_SYSTEMS_RENDER_ZORDERQUEUE_HPP__
#define SRC_SYSTEMS_RENDER_ZORDERQUEUE_HPP__

#include <PathFinding/SmartPointers.hpp>

#include <vector>

// clang-format off
namespace Game::Cmp { class ZOrderValue; }
// clang-format on

namespace Game::Sys
{

//! @brief The list of visible renderable entities, sorted lowest z-order first. Rebuilt every frame by
//! RenderGameSystem, which then draws the entries in order.
class ZOrderQueue
{
public:
  //! @brief Z-order entry for rendering queue
  struct Entry
  {
    //! @brief Z-order value; lower values are drawn first.
    float z;
    //! @brief The entity being ordered.
    entt::entity e;
  };

  //! @brief Clears the queue, then re-populates and sorts it with the latest entity/component data
  //! @param reg The registry to queue entities from
  //! @param view_bounds The world-space bounds that entities are visibility-tested against
  //! @param render_position_grid Optional spatial index of static (never moved after creation)
  //! Cmp::Position-bearing renderable entities - see queue_positioned(). nullptr (the default) falls
  //! back to the unindexed full-registry scan.
  void refresh( entt::registry &reg, sf::FloatRect view_bounds, const PathFinding::SpatialHashGridSharedPtr &render_position_grid = nullptr );

  //! @brief The queued entries, lowest z-order first
  [[nodiscard]] const std::vector<Entry> &entries() const { return m_entries; }

  [[nodiscard]] auto begin() const { return m_entries.begin(); }
  [[nodiscard]] auto end() const { return m_entries.end(); }
  [[nodiscard]] std::size_t size() const { return m_entries.size(); }
  [[nodiscard]] bool empty() const { return m_entries.empty(); }

private:
  //! @brief Queues every visible multiblock root in the list. The whole multiblock rect is visibility-tested,
  //! preventing pop-in when only part of it is inside the view.
  template <typename... MultiBlock>
  void queue_multiblocks( entt::registry &reg, sf::FloatRect view_bounds, entt::type_list<MultiBlock...> );

  //! @brief Queues every entity with CmpT, without a visibility test. For components with no world bounds.
  template <typename CmpT>
  void queue_all( entt::registry &reg );

  //! @brief Queues world-space particle sprites whose bounds are in view, and all screen-space ones.
  void queue_particles( entt::registry &reg, sf::FloatRect view_bounds );

  //! @brief Queues every visible Cmp::Position entity, other than multiblock roots.
  //! @param render_position_grid Spatial index of static entities, queried instead of scanning the registry;
  //! movers are then scanned separately. nullptr falls back to a full registry scan.
  void queue_positioned( entt::registry &reg, sf::FloatRect view_bounds, const PathFinding::SpatialHashGridSharedPtr &render_position_grid );

  //! @brief Queues visible Cmp::Position entities that also have every component in Filter.
  //! An empty Filter scans every positioned entity.
  template <typename... Filter>
  void queue_positioned_view( entt::registry &reg, sf::FloatRect view_bounds );

  //! @brief Appends an entity to the queue
  void push( entt::entity entity, const Cmp::ZOrderValue &z_order_cmp );

  //! @brief The queued entries. Sorted lowest z-order first once refresh() returns.
  std::vector<Entry> m_entries;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_ZORDERQUEUE_HPP__
