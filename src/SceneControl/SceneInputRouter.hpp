#ifndef SRC_SCENECONTROL_SCENEINPUTROUTER_HPP__
#define SRC_SCENECONTROL_SCENEINPUTROUTER_HPP__

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Window.hpp>

#include <string_view>

#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <Systems/BaseSystem.hpp>

// clang-format off
namespace Game::Sys { class Store; }
namespace Game::Audio { class SoundBank; }
// clang-format on

namespace Game::Sys
{

//! @brief Polls window/keyboard/mouse input and translates it into gameplay events and
//!        SceneManagerEvent navigation requests. Each `*_scene_state_handler()` method is wired
//!        up (via m_nav_event_dispatcher) to run only while its corresponding scene is active.
class SceneInputRouter : public Sys::BaseSystem
{
public:
  //! @brief High-level navigation intents that can result from scene input handling.
  enum class NavigationActions {
    //! @brief No navigation action requested
    NONE,
    //! @brief Navigate to the title scene
    TITLE,
    //! @brief Navigate to the settings menu scene
    SETTINGS,
    //! @brief Navigate to the crypt scene
    CRYPT,
    //! @brief Pause the game
    PAUSE,
    //! @brief Resume the game
    RESUME,
    //! @brief Navigate to the game over scene
    GAMEOVER,
    //! @brief Exit the game
    EXIT
  };

  //! @brief Construct a new Scene Input Router object
  //! @param reg Entity-component registry for the current scene
  //! @param window The render window to poll input events from
  //! @param sound_bank Used to play sounds in response to input
  //! @param nav_event_dispatcher Dispatcher used to route the per-scene "process input" events to the matching handler
  //! @param scenemanager_event_dispatcher Dispatcher used to request scene transitions from SceneManager
  SceneInputRouter( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, entt::dispatcher &nav_event_dispatcher,
                    entt::dispatcher &scenemanager_event_dispatcher );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief Process input while the title scene is active.
  void title_scene_input_handler();
  //! @brief Process input while the settings menu scene is active.
  void settings_scene_state_handler();
  //! @brief Process input while the graveyard scene is active.
  void graveyard_scene_state_handler();
  //! @brief Process input while the crypt scene is active.
  void crypt_scene_state_handler();
  //! @brief Process input while the healing spring scene is active.
  void healing_spring_scene_state_handler();
  //! @brief Process input while the shop scene is active.
  void shop_scene_state_handler();
  //! @brief Process input while a ruin scene (upper or lower floor) is active.
  void ruin_scene_state_handler();
  //! @brief Process input while the paused menu scene is active.
  void paused_scene_state_handler();
  //! @brief Process input while the game over scene is active.
  void game_over_scene_state_handler();
  //! @brief Process input while the level complete scene is active.
  void level_complete_scene_state_handler();

  //! @brief Dispatcher used to route the per-scene "process input" events to the matching handler.
  entt::dispatcher &m_nav_event_dispatcher;
  //! @brief Dispatcher used to request scene transitions from SceneManager.
  entt::dispatcher &m_scenemanager_event_dispatcher;

private:
  //! @brief reset and process new user direction input
  void process_move_keys();

  //! @brief forwards to ImGui and handles the window Closed/Resized events common to every scene; returns true if the event was consumed
  bool dispatch_common_window_event( const sf::Event &event );
  //! @brief handles the debug/cheat keys shared by every gameplay scene; returns true if the scancode was handled
  bool try_handle_debug_key( sf::Keyboard::Scancode scancode );

  //! @brief Resize the render window's view to match the new window size.
  //! @param size New window size in pixels
  void resize_window( sf::Vector2u size );

  //! @brief Toggle a boolean scene-setting component's `enabled` flag and log the new state.
  //! @tparam SettingComponent The scene-setting component type (e.g. Cmp::SceneSettings::ShowNavmesh)
  //! @param log_prefix Text prefixed to the logged ENABLED/DISABLED message
  template <typename SettingComponent>
  void toggle_setting( std::string_view log_prefix );

  //! @brief Queue a PlayerMortalityEvent that kills the player via suicide (debug/cheat key).
  void queue_suicide_event();

  //! @brief Queue a BuyShopItemEvent for the given shop slot index.
  //! @param item_idx Index of the shop item slot to buy
  void queue_buy_item_event( uint8_t item_idx );

  //! @brief Queue a SceneManagerEvent to quit back out of the current gameplay scene, unless the
  //!        player is mid-death animation.
  void queue_quit_game_event();

  //! @brief Enqueue a SceneManagerEvent of the given type on m_scenemanager_event_dispatcher.
  //! @param ev The scene manager event type to enqueue
  void enqueue( Events::SceneManagerEvent::Type type );

  //! @brief Enable or disable every Cmp::Particle::SpriteOwner's particle sprite (debug/cheat key).
  //! @param enable true to restart the particle sprites, false to stop them
  void toggle_particle_test( bool enable );
};

} // namespace Game::Sys

#endif // SRC_SCENECONTROL_SCENEINPUTROUTER_HPP__
