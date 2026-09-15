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
namespace Game::Sprites { class SpriteFactory; }
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
  //! @param sprite_factory
  //! @param sound_bank
  BombSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

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
  void update();

  //! @brief event handlers for pausing system clocks
  void on_pause() override;
  //! @brief event handlers for resuming system clocks
  void on_resume() override;

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

  //! @brief Arm a bomb centered on the player's current position (used for grave bombs).
  void arm_grave_bomb();

  //! @brief Arm a bomb on the destructable tile the player is currently standing on, consuming a bomb item from the player's inventory.
  void arm_player_bomb();

  //! @brief Arm a bomb centered on the given target entity (used for chain-reacting nearby explosives).
  //! @param target_entt
  void arm_entt( entt::entity target_entt );
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_BOMBSYSTEM_HPP__
