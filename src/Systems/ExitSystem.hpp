#ifndef SRC_SYSTEMS_EXITSYSTEM_HPP__
#define SRC_SYSTEMS_EXITSYSTEM_HPP__

#include <Events/PlayerActionEvent.hpp>
#include <Events/UnlockDoorEvent.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/System/Vector2.hpp>

namespace Game::Sys
{

//! @brief Graveyard exit only
class ExitSystem : public BaseSystem
{
public:
  //! @brief Construct a new Exit System object
  //! @param reg
  //! @param window
  //! @param sprite_factory
  //! @param sound_bank
  //! @param scenemanager_event_dispatcher
  ExitSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank,
              entt::dispatcher &scenemanager_event_dispatcher );

  //! @brief init the weak pointer for the reserved-positions grid, ahead of exit placement.
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &reserved_sm ) { m_reserved_sm = reserved_sm; }

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief Event handler for unlocking the exit
  //! @param ev
  void on_player_action( Events::PlayerActionEvent ev );

  //! @brief Create the exit in the game area
  //! @param spawn_position
  void create_exit();

  //! @brief Check for player collision with open exit to complete level
  void check_exit_collision();

  //! @brief Update exit multiblock z-order using the segment zorder.
  void update_exit_zorder();

private:
  //! @brief Special requirement to signal Events::SceneManagerEvent::Type::LEVEL_COMPLETE to the Scene Manager
  entt::dispatcher &m_scenemanager_event_dispatcher;

  //! @brief  This function is called via event trigger:
  //!         `Events::PlayerActionEvent` from `SceneInputRouter::graveyard_scene_state_handler()`
  void unlock_exit();

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_EXITSYSTEM_HPP__
