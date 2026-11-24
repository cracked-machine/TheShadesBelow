#ifndef SRC_SYS_CARTHORSESYSTEM_HPP__
#define SRC_SYS_CARTHORSESYSTEM_HPP__

#include <Events/PlayerActionEvent.hpp>
#include <SFML/System/Time.hpp>
#include <Systems/BaseSystem.hpp>
namespace Game::Sys
{

//! @brief Graveyard exit only
class CartHorseSystem : public BaseSystem
{
public:
  //! @brief Construct a new Exit System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  //! @param scenemanager_event_dispatcher
  CartHorseSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, entt::dispatcher &scenemanager_event_dispatcher );

  void update( sf::Time dt );

  //! @brief Event handler for unlocking the exit
  //! @param ev
  void on_player_action( Events::PlayerActionEvent ev );

private:
  //! @brief True if the player is next to `bounds` and facing it.
  //! @param bounds The world bounds to test.
  [[nodiscard]] bool is_player_facing( const sf::FloatRect &bounds );

  //! @brief Move one of the selected inventory item into the first barrel whose matching cadaver part slot is empty.
  //! @return true if an item was transferred.
  bool transfer_selected_item();

  //! @brief Special requirement to signal Events::SceneManagerEvent::Type::LEVEL_COMPLETE to the Scene Manager
  entt::dispatcher &m_scenemanager_event_dispatcher;

  sf::Time m_activate_timer{ sf::Time::Zero };
  sf::Time m_activate_timeout{ sf::seconds( 1.f ) };
};

} // namespace Game::Sys
#endif // SRC_SYS_CARTHORSESYSTEM_HPP__