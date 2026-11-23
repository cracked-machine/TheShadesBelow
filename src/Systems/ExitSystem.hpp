#ifndef SRC_SYSTEMS_EXITSYSTEM_HPP__
#define SRC_SYSTEMS_EXITSYSTEM_HPP__

#include <Events/PlayerActionEvent.hpp>
#include <Events/UnlockDoorEvent.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/System/Vector2.hpp>

namespace Game::Cmp
{
class Exit;
}

namespace Game::Sys
{

//! @brief Graveyard exit only
class ExitSystem : public BaseSystem
{
public:
  //! @brief Construct a new Exit System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  //! @param scenemanager_event_dispatcher
  ExitSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, entt::dispatcher &scenemanager_event_dispatcher );

  //! @brief init the weak pointers for the reserved-positions grid and world-tile lookup, ahead of exit placement.
  //! @param reserved_sm
  //! @param world_tile_sm World-tile lookup (see Scene::m_world_tile_sm); null falls back to scanning the registry.
  void init( const PathFinding::SpatialHashGridSharedPtr &reserved_sm, const PathFinding::SpatialHashGridSharedPtr &world_tile_sm = nullptr )
  {
    m_reserved_sm = reserved_sm;
    m_world_tile_sm = world_tile_sm;
  }

  void update( sf::Time dt );

  //! @brief Event handler for unlocking the exit
  //! @param ev
  void on_player_action( Events::PlayerActionEvent ev );

  //! @brief Create the exit multiblock in the game area
  //! @param pos Top-left pixel position of the exit multiblock, e.g. a 'poi' marker from the scene data.
  void create_exit( sf::Vector2f pos );

  //! @brief Check for player collision with open exit to complete level
  void check_exit_collision();

private:
  //! @brief Special requirement to signal Events::SceneManagerEvent::Type::LEVEL_COMPLETE to the Scene Manager
  entt::dispatcher &m_scenemanager_event_dispatcher;

  //! @brief  This function is called via event trigger:
  //!         `Events::PlayerActionEvent` from `SceneInputRouter::graveyard_scene_state_handler()`
  void unlock_exit();

  //! @brief The exit the player is standing in front of, or nullptr.
  Cmp::Exit *player_at_exit();

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;

  //! @brief World-tile lookup, used to find the tile under the exit without a registry scan.
  PathFinding::SpatialHashGridWeakPtr m_world_tile_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_EXITSYSTEM_HPP__
