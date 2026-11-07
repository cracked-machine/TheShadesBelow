#ifndef SRC_SYSTEMS_THREATS_BOMBSYSTEM_HPP__
#define SRC_SYSTEMS_THREATS_BOMBSYSTEM_HPP__

#include <Systems/BaseSystem.hpp>
#include <Systems/Threats/NpcSystem.hpp>
#include <Utils/Constants.hpp>

#include <entt/entity/fwd.hpp>

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <Events/PlayerActionEvent.hpp>
#include <spdlog/spdlog.h>

// clang-format off
namespace Game::Sys { class Store; }
namespace Game::Audio { class SoundBank; }
// clang-format on

namespace Game::PathFinding
{
class SpatialHashGrid;
}

namespace Game::Sys
{

//! @brief Manages arming, fuse timing and detonation of bombs and their explosion effects.
//! @note This currently only supports one bomb at a time.
class BombSystem : public BaseSystem
{
public:
  //! @brief Construct a new Bomb System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  BombSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointer for the pathfinding navmesh
  //! @param npc_navmesh
  void init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh, const PathFinding::SpatialHashGridSharedPtr &player_navmesh,
             const PathFinding::SpatialHashGridSharedPtr &ghost_navmesh = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr )
  {
    m_npc_navmesh = npc_navmesh;
    m_player_navmesh = player_navmesh;
    m_ghost_navmesh = ghost_navmesh;
    m_reserved_sm = reserved_sm;
  }

  //! @brief Detonate any armed bombs whose fuse has expired: destroys obstacles, loot containers, npc containers and nearby items, damages the
  //! player and npcs caught in the blast, triggers chain reactions with other explosives, and replaces the bomb with a detonated sprite.
  //! @param dt Frame delta, accumulated into each Cmp::Armed fuse/warning timer
  void update( sf::Time dt );

private:
  //! @brief Weak pointer to the NPC pathfinding navmesh, updated when obstacles are destroyed by explosions.
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief Weak pointer to the ghost pathfinding navmesh, updated when obstacles are destroyed by explosions.
  PathFinding::SpatialHashGridWeakPtr m_ghost_navmesh;

  //! @brief Weak pointer to the player pathfinding navmesh, updated when obstacles are destroyed by explosions.
  PathFinding::SpatialHashGridWeakPtr m_player_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;

  //! @brief Recursively arm all destructable entities in concentric rings out from an epicenter, up to blast_radius layers, in clockwise order per
  //! layer.
  //! @param epicenter_entity
  //! @param blast_radius
  //! @param depth Current recursion depth; recursion stops once it reaches the internal max recursion depth.
  void place_concentric_bomb_pattern( const entt::entity &epicenter_entity, int blast_radius );

  //! @brief Dispatches DROP_BOMB and GRAVE_BOMB player actions to arm_player_bomb() and arm_grave_bomb() respectively.
  //! @param event
  void on_bomb_event( const Events::PlayerActionEvent &event );

  //! @brief Arm a grave bomb centered on source_entt's position, or on the player's current position if it has none.
  //! @param source_entt The grave that set the bomb off when it was not dug by the player, else entt::null.
  void arm_grave_bomb( entt::entity source_entt );

  //! @brief Arm a bomb on the destructable tile the player is currently standing on, consuming a bomb item from the player's inventory.
  void arm_player_bomb();

  //! @brief Arm a bomb centered on the given target entity (used for chain-reacting nearby explosives).
  //! @param target_entt
  void arm_entt( entt::entity target_entt );

  //! @brief Blast radius (tiles) for player-placed bombs: higher luck means a bigger explosion.
  //! @note Tuning constants are defined in BombSystem.cpp.
  //! @return int The blast radius, at least 1.
  int player_blast_radius();

  //! @brief Blast radius (tiles) for grave bombs: lower luck means a bigger explosion.
  //! Grave bombs are rare at high luck, so the range is concentrated in the low end.
  //! @note Tuning constants are defined in BombSystem.cpp.
  //! @return int The blast radius.
  int grave_blast_radius();
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_BOMBSYSTEM_HPP__
