#ifndef SRC_SYSTEMS_LOOTSYSTEM_HPP__
#define SRC_SYSTEMS_LOOTSYSTEM_HPP__

#include <Components/LootContainer.hpp>
#include <Events/DestroyLootContainerEvent.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sys
{

//! @brief Handles breaking Cmp::LootContainer entities (pots) and spawning their loot drops, in response to
//! Events::DestroyLootContainerEvent, and player pickup of dropped loot.
class LootSystem : public BaseSystem
{
public:
  //! @brief Construct a new Loot System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  LootSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointers for the pathfinding navmeshes
  //! @param npc_navmesh
  //! @param player_navmesh
  //! @param ghost_navmesh
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh, const PathFinding::SpatialHashGridSharedPtr &player_navmesh,
             const PathFinding::SpatialHashGridSharedPtr &ghost_navmesh = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr )
  {
    m_npc_navmesh = npc_navmesh;
    m_player_navmesh = player_navmesh;
    m_ghost_navmesh = ghost_navmesh;
    m_reserved_sm = reserved_sm;
  }

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief Check for player collision with loot items i.e. player picks up loot
  void check_loot_collision( sf::Time dt );

  //! @brief Destroy loot container particle sprites once all their particles have expired
  void update_loot_container_particles();

private:
  //! @brief Break the loot container, then 50% chance of dropping random loot in its place
  //! @param event
  void on_destroy_loot_container_event( const Events::DestroyLootContainerEvent &event );

  //! @brief all grid positions that block NPC pathfinding
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief All grid positions that block ghost NPC pathfinding
  PathFinding::SpatialHashGridWeakPtr m_ghost_navmesh;

  //! @brief All grid positions that block player movement
  PathFinding::SpatialHashGridWeakPtr m_player_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_LOOTSYSTEM_HPP__
