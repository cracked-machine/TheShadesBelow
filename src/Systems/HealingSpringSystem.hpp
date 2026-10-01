#ifndef SRC_SYSTEMS_SACREDSPRINGSYSTEM_HPP__
#define SRC_SYSTEMS_SACREDSPRINGSYSTEM_HPP__

#include <Events/PlayerActionEvent.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sys
{

//! @brief Handles the healing spring building: entrance/exit transitions, z-order, and depositing the
//! curse tablet for wealth.
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

  //! @brief Capture Events::PlayerActionEvent::GameActions::ACTIVATE. Calls check_inventory_deposit().
  //! @param ev
  void on_player_action( Events::PlayerActionEvent ev );

  //! @brief Sync the building multiblock's z-order to the segment the player is currently standing on.
  void update_building_zorder();

  //! @brief Check for collisions with the entrance
  void check_entrance_collision();

  //! @brief Check for collisions with the exit
  void check_exit_collision();

  //! @brief Deposit the player's curse tablet at the spring for wealth, if they are carrying one and standing near it.
  void check_inventory_deposit();

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Dispatcher reference for scene management events
  entt::dispatcher &m_scenemanager_event_dispatcher;

  //! @brief Time since the last inventory deposit check.
  sf::Time m_inventory_deposit_interval{};
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_SACREDSPRINGSYSTEM_HPP__
