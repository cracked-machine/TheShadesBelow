#ifndef SRC_SYSTEMS_SACREDSPRINGSYSTEM_HPP__
#define SRC_SYSTEMS_SACREDSPRINGSYSTEM_HPP__

#include <Events/PlayerActionEvent.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/System/Time.hpp>

#include <optional>

namespace Game::Sys
{

//! @brief Handles the graveyard healing building (the barber surgeon): entrance/exit transitions, z-order, and
//! depositing the curse tablet for wealth.
class HealingSpringSystem : public BaseSystem
{
public:
  //! @brief Construct a new Healing Spring System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  //! @param scenemanager_event_dispatcher
  HealingSpringSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, entt::dispatcher &scenemanager_event_dispatcher )
      : Game::Sys::BaseSystem( reg, window, sound_bank ),
        m_scenemanager_event_dispatcher( scenemanager_event_dispatcher )
  {
    std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&HealingSpringSystem::on_player_action>( this );
  }

  void update( sf::Time dt );

  //! @brief Capture Events::PlayerActionEvent::GameActions::ACTIVATE. Calls check_inventory_deposit().
  //! @param ev
  void on_player_action( Events::PlayerActionEvent ev );

  //! @brief Check for collisions with the entrance. Enters the barber surgeon scene.
  void check_entrance_collision();

  //! @brief Check for collisions with the exit
  //! @param exit_type The scene manager event to enqueue when the player reaches the exit
  void check_exit_collision( Events::SceneManagerEvent::Type exit_type = Events::SceneManagerEvent::Type::EXIT_BARBERSURGEON );

  //! @brief Leave the barber surgeon scene after a short delay, e.g. after a paid heal.
  //! No-op if an exit is already pending.
  //! @param exit_type The scene manager event to enqueue when the delay ends
  void request_delayed_exit( Events::SceneManagerEvent::Type exit_type );

  //! @brief Advance the delayed exit countdown, enqueuing the exit event when it ends. Call every frame from the scene.
  //! @param dt
  void update_delayed_exit( sf::Time dt );

  //! @brief Drop any pending delayed exit, e.g. when the scene is left another way during the countdown.
  void cancel_delayed_exit()
  {
    m_pending_exit.reset();
    m_exit_delay_remaining = sf::Time::Zero;
  }

  //! @brief True while a delayed exit is counting down.
  [[nodiscard]] bool is_exit_pending() const { return m_pending_exit.has_value(); }

  //! @brief Deposit the player's curse tablet at the barber surgeon's fountain for wealth, if they are carrying one and standing near it.
  void check_inventory_deposit();

private:
  //! @brief Dispatcher reference for scene management events
  entt::dispatcher &m_scenemanager_event_dispatcher;

  //! @brief Time since the last inventory deposit check.
  sf::Time m_inventory_deposit_interval;

  //! @brief The exit event to enqueue once m_exit_delay_remaining runs out, if a delayed exit is pending.
  std::optional<Events::SceneManagerEvent::Type> m_pending_exit;

  //! @brief Time left before the pending exit is enqueued.
  sf::Time m_exit_delay_remaining;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_SACREDSPRINGSYSTEM_HPP__
