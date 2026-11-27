#ifndef SRC_SCENECONTROL_SCENEINPUTROUTER_HPP__
#define SRC_SCENECONTROL_SCENEINPUTROUTER_HPP__

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Window.hpp>

#include <string_view>

#include <Events/PlayerActionEvent.hpp>
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

  //! @brief Process input while the title scene is active.
  void title_scene_input_handler();
  //! @brief Process input while the settings menu scene is active.
  void settings_scene_state_handler();
  //! @brief Process input while the graveyard scene is active.
  void graveyard_scene_state_handler();
  //! @brief Process input while the crypt scene is active.
  void crypt_scene_state_handler();
  //! @brief Process input while the barber surgeon scene is active.
  void barber_surgeon_scene_state_handler();
  //! @brief Process input while the shop scene is active.
  void shop_scene_state_handler();
  //! @brief Process input while a ruin scene (upper or lower floor) is active.
  void ruin_scene_state_handler();
  //! @brief Process input while the game over scene is active.
  void game_over_scene_state_handler();
  //! @brief Process input while the level complete scene is active.
  void level_complete_scene_state_handler();

  //! @brief Dispatcher used to route the per-scene "process input" events to the matching handler.
  entt::dispatcher &m_nav_event_dispatcher;
  //! @brief Dispatcher used to request scene transitions from SceneManager.
  entt::dispatcher &m_scenemanager_event_dispatcher;

private:
  //! @brief Where the E key is in confirming the leave prompt. The prompt is opened by pressing E, and that E is still
  //! held (and key-repeating) as the prompt opens, so only a fresh press-and-release of E while it is open confirms.
  enum class LeaveConfirmKey
  {
    //! @brief The prompt is not open: work out the state the next time it is.
    UNKNOWN,
    //! @brief The E that opened the prompt is still held: ignore it until it is released.
    HELD_FROM_OPEN,
    //! @brief E is up: the next press arms it.
    READY,
    //! @brief E was pressed while the prompt was open: its release confirms.
    ARMED
  };
  //! @brief See LeaveConfirmKey. Reset to UNKNOWN whenever the graveyard handler runs with the prompt closed.
  LeaveConfirmKey m_leave_confirm_key{ LeaveConfirmKey::UNKNOWN };

  //! @brief reset and process new user direction input
  void process_move_keys();

  //! @brief The inventory swap action for the E key: SWAP_INVENTORY_ALL while Shift is held (drop the whole stack),
  //! otherwise SWAP_INVENTORY (drop one).
  [[nodiscard]] static Events::PlayerActionEvent::GameActions swap_inventory_action();

  //! @brief Process input while the pause overlay is shown. Gameplay is paused, so only P (resume), the volume sliders
  //!        (via ImGui) and the common window events are handled.
  void pause_overlay_input_handler();
  //! @brief Check whether the pause menu is open (see OverlayArbitrationSystem::active_modal()).
  //! @return true if paused, false if not or the scene has no pause setting.
  bool is_paused();
  //! @brief Trigger a PAUSE_TOGGLE overlay event.
  void toggle_pause();

  //! @brief Process input while the grimoire overlay is open. Gameplay is paused, so only the grimoire's own
  //!        controls (process_grimoire_keys, process_grimoire_wheel) and the common window events are handled.
  void grimoire_overlay_input_handler();
  //! @brief Check whether the player's grimoire overlay is open in the current scene's registry.
  //! @return true if the grimoire is open, false if it is closed or there is no player.
  bool is_grimoire_open();
  //! @brief Process input while a pausing tooltip is shown. Gameplay is paused, so only Enter (dismiss the tooltip) and
  //!        the common window events are handled.
  void tooltip_pause_input_handler();
  //! @brief Process input while the "leave the graveyard?" prompt is open. Gameplay is paused, so only the prompt's own
  //!        controls (Up/Down/W/S/wheel to choose, E to confirm, Y/N/Escape to answer) and the common window events
  //!        are handled. See LeaveConfirmKey.
  void leave_prompt_input_handler();
  //! @brief Check whether the "leave the graveyard?" prompt is open (see OverlayArbitrationSystem::active_modal()).
  //! @return true if the prompt is open
  bool is_leave_prompt_open();
  //! @brief Check whether a pausing tooltip is shown (see OverlayArbitrationSystem::active_modal()).
  //! @return true if gameplay is paused by a tooltip
  bool is_tooltip_paused();

  //! @brief Trigger the held-key grimoire actions: G toggles the overlay, Left/Right turn pages.
  void process_grimoire_keys();

  //! @brief Turn the grimoire page if the event is a vertical mouse wheel scroll; otherwise does nothing.
  //! @param event The polled window event
  void process_grimoire_wheel( const sf::Event &event );

  //! @brief Route a vertical mouse wheel event to the heal menu (Events::OverlayEvent HEAL_PREV/HEAL_NEXT).
  //! @param event the window event
  //! @return true if the event was a vertical wheel event and was consumed
  bool process_heal_menu_wheel( const sf::Event &event );

  //! @brief Change the selected inventory slot: a vertical mouse wheel scroll selects the previous/next slot, and (if
  //!        `number_keys`) the number row keys 1-5 select a slot directly. Numpad keys are never used.
  //! @param event The polled window event
  //! @param number_keys false in scenes that use the number keys for something else (e.g. buying in the shop)
  void process_inventory_slot_input( const sf::Event &event, bool number_keys );

  //! @brief Drag items between inventory slots with the left mouse button (see Cmp::InventoryDrag): pressing on an
  //!        occupied slot picks its item up, releasing on another slot sends Events::MoveInventorySlotEvent and releasing
  //!        off the inventory sends Events::DropInventorySlotEvent.
  //! @param event The polled window event
  //! @return true if the event was a left button press on an inventory slot, or the release that ends a drag, so it
  //!         must not also draw or release the bow
  bool process_inventory_drag( const sf::Event &event );

  //! @brief Check whether an inventory drag is in progress or the mouse is over an inventory slot, so the held left
  //!        button must not dig, attack or burn.
  //! @return true if the mouse belongs to the inventory
  bool mouse_on_inventory();

  //! @brief Forward the event to ImGui. Safe in gameplay scenes: OverlayArbitrationSystem pumps an ImGui frame every frame.
  //! @param event The polled window event
  void forward_to_imgui( const sf::Event &event );
  //! @brief Check whether the mouse is over (or dragging) an overlay window that takes input (e.g. the F3 debug panels),
  //! so gameplay should ignore mouse input.
  //! @return true if ImGui wants the mouse
  static bool imgui_wants_mouse();
  //! @brief Check whether the event is a mouse button press/release or wheel scroll.
  //! @param event The polled window event
  //! @return true for mouse button and wheel events
  static bool is_mouse_event( const sf::Event &event );

  //! @brief handles the window Closed/Resized events common to every scene; returns true if the event was consumed
  bool dispatch_common_window_event( const sf::Event &event );
  //! @brief handles the debug/cheat keys shared by every gameplay scene; returns true if the scancode was handled
  void try_handle_debug_key( sf::Keyboard::Scancode scancode );

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
