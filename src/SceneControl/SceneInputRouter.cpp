
#include <Audio/SoundBank.hpp>
#include <Components/Direction.hpp>
#include <Components/Exit.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Persistent/ExitKeyRequirement.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/Cadaver.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/ConsumeTimer.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/SceneSettings/Footsteps.hpp>
#include <Components/SceneSettings/Shaders.hpp>
#include <Components/SceneSettings/ShowNavmesh.hpp>
#include <Components/SceneSettings/ShowPathFinding.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Components/Toxicity/Venom.hpp>
#include <Events/BuyShopItemEvent.hpp>
#include <Events/CryptRoomEvent.hpp>
#include <Events/DismissTooltipEvent.hpp>
#include <Events/DropInventoryEvent.hpp>
#include <Events/LightningEvent.hpp>
#include <Events/LoadSettingsEvent.hpp>
#include <Events/OverlayEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Events/SaveSettingsEvent.hpp>
#include <Events/SelectInventorySlotEvent.hpp>
#include <Events/UnlockDoorEvent.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Factory/ToxicityFactory.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SceneControl/Events/ProcessCryptSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessGameoverSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessGraveyardSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessBarberSurgeonSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessHealingSpringSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessLevelCompleteSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessRuinSceneLowerInputEvent.hpp>
#include <SceneControl/Events/ProcessRuinSceneUpperInputEvent.hpp>
#include <SceneControl/Events/ProcessSettingsMenuSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessShopSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessTitleSceneInputEvent.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <SceneControl/SceneInputRouter.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/OverlayArbitrationSystem.hpp>
#include <Systems/ShopSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <algorithm>
#include <array>
#include <imgui-SFML.h>
#include <imgui.h>

namespace Game::Sys
{

SceneInputRouter::SceneInputRouter( entt::registry &reg, sf::RenderWindow &m_window, Audio::SoundBank &sound_bank,
                                    entt::dispatcher &nav_event_dispatcher, entt::dispatcher &scenemanager_event_dispatcher )
    : Sys::BaseSystem( reg, m_window, sound_bank ),
      m_nav_event_dispatcher( nav_event_dispatcher ),
      m_scenemanager_event_dispatcher( scenemanager_event_dispatcher )
{
  // clang-format off
  m_nav_event_dispatcher.sink<Events::ProcessTitleSceneInputEvent>().connect<&SceneInputRouter::title_scene_input_handler>( this );
  m_nav_event_dispatcher.sink<Events::ProcessSettingsMenuSceneInputEvent>().connect<&SceneInputRouter::settings_scene_state_handler>( this );
  m_nav_event_dispatcher.sink<Events::ProcessGraveyardSceneInputEvent>().connect<&SceneInputRouter::graveyard_scene_state_handler>(this );
  m_nav_event_dispatcher.sink<Events::ProcessGameoverSceneInputEvent>().connect<&SceneInputRouter::game_over_scene_state_handler>(this );
  m_nav_event_dispatcher.sink<Events::ProcessLevelCompleteSceneInputEvent>().connect<&SceneInputRouter::level_complete_scene_state_handler>( this );
  m_nav_event_dispatcher.sink<Events::ProcessCryptSceneInputEvent>().connect<&SceneInputRouter::crypt_scene_state_handler>( this );
  m_nav_event_dispatcher.sink<Events::ProcessHealingSpringSceneInputEvent>().connect<&SceneInputRouter::healing_spring_scene_state_handler>( this );
  m_nav_event_dispatcher.sink<Events::ProcessBarberSurgeonSceneInputEvent>().connect<&SceneInputRouter::healing_spring_scene_state_handler>( this );
  m_nav_event_dispatcher.sink<Events::ProcessRuinSceneLowerInputEvent>().connect<&SceneInputRouter::ruin_scene_state_handler>( this );
  m_nav_event_dispatcher.sink<Events::ProcessRuinSceneUpperInputEvent>().connect<&SceneInputRouter::ruin_scene_state_handler>( this );
  m_nav_event_dispatcher.sink<Events::ProcessShopSceneInputEvent>().connect<&SceneInputRouter::shop_scene_state_handler>( this );
  // clang-format on
}

void SceneInputRouter::title_scene_input_handler()
{
  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::Enter ) { enqueue( Events::SceneManagerEvent::Type::START_GAME ); }
      else if ( keyPressed->scancode == sf::Keyboard::Scancode::Q ) { enqueue( Events::SceneManagerEvent::Type::EXIT_GAME ); }
      else if ( keyPressed->scancode == sf::Keyboard::Scancode::S ) { enqueue( Events::SceneManagerEvent::Type::SETTINGS_MENU ); }
    }
  }
}

void SceneInputRouter::settings_scene_state_handler()
{
  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    // Only scenes that actually pump an ImGui frame (via ImGui::SFML::Update/Render in
    // RenderMenuSystem) may forward events here. Feeding events into ImGui's queue while
    // no NewFrame() is being called lets them pile up and get replayed onto menu widgets
    // the next time a menu opens - which looks like widgets changing on their own.
    ImGui::SFML::ProcessEvent( m_window, *event );
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::Escape ) { enqueue( Events::SceneManagerEvent::Type::EXIT_SETTINGS_MENU ); }
      else if ( keyPressed->scancode == sf::Keyboard::Scancode::U ) { get_systems_event_queue().trigger( Events::LoadSettingsEvent() ); }
    }
  }
}

void SceneInputRouter::graveyard_scene_state_handler()
{
  if ( is_paused() )
  {
    pause_overlay_input_handler();
    return;
  }
  if ( is_grimoire_open() )
  {
    grimoire_overlay_input_handler();
    return;
  }
  if ( is_tooltip_paused() )
  {
    tooltip_pause_input_handler();
    return;
  }

  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    forward_to_imgui( *event );
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( is_mouse_event( *event ) and imgui_wants_mouse() ) continue;
    process_inventory_slot_input( *event, /*number_keys=*/true );
    if ( const auto *keyReleased = event->getIf<sf::Event::KeyReleased>() )
    {
      try_handle_debug_key( keyReleased->scancode );
      if ( keyReleased->scancode == sf::Keyboard::Scancode::Enter ) { get_systems_event_queue().trigger( Events::DismissTooltipEvent() ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Home ) { toggle_particle_test( true ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::End ) { toggle_particle_test( false ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Escape ) { queue_quit_game_event(); }
    }
    else if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::P ) { toggle_pause(); }
    }
    else if ( const auto *mouseReleased = event->getIf<sf::Event::MouseButtonReleased>() )
    {
      if ( mouseReleased->button == sf::Mouse::Button::Left )
      {
        get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::RELEASE_BOW ) );
      }
    }
    else if ( const auto *mousePressed = event->getIf<sf::Event::MouseButtonPressed>() )
    {
      if ( mousePressed->button == sf::Mouse::Button::Left )
      {
        get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::DRAW_BOW ) );
      }
    }
  }

  process_move_keys();

  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Space ) )
  {
    // no conflict: bombs need item.bomb in the inventory slot, flight needs item.witchesbroom
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::PLACE_BOMB ) );
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::FLY ) );
  }
  process_grimoire_keys();
  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::F ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::CONSUME ) );
  }
  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::E ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ACTIVATE ) );
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::SWAP_INVENTORY ) );
  }
  if ( not imgui_wants_mouse() and sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
  {
    // get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::DRAW_BOW ) );
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::BURN ) );
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ATTACK ) );
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::DIG ) );
  }
}

void SceneInputRouter::crypt_scene_state_handler()
{
  if ( is_paused() )
  {
    pause_overlay_input_handler();
    return;
  }
  if ( is_tooltip_paused() )
  {
    tooltip_pause_input_handler();
    return;
  }

  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    forward_to_imgui( *event );
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( is_mouse_event( *event ) and imgui_wants_mouse() ) continue;
    process_inventory_slot_input( *event, /*number_keys=*/true );
    if ( const auto *keyReleased = event->getIf<sf::Event::KeyReleased>() )
    {
      try_handle_debug_key( keyReleased->scancode );
      if ( keyReleased->scancode == sf::Keyboard::Scancode::Enter ) { get_systems_event_queue().trigger( Events::DismissTooltipEvent() ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Escape )
      {
        // Prevent skipping the death animation and leaving the game in a bad state
        if ( Utils::Player::get_mortality( reg() ).state != Cmp::Player::Mortality::State::ALIVE ) continue;
        enqueue( Events::SceneManagerEvent::Type::EXIT_CRYPT );
      }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::R )
      {
        get_systems_event_queue().trigger( Events::CryptRoomEvent( Events::CryptRoomEvent::Type::SHUFFLE_PASSAGES ) );
      }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::T )
      {
        get_systems_event_queue().trigger( Events::CryptRoomEvent( Events::CryptRoomEvent::Type::FINAL_PASSAGE ) );
      }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Y )
      {
        get_systems_event_queue().trigger( Events::CryptRoomEvent( Events::CryptRoomEvent::Type::EXIT_ALL_PASSAGES ) );
      }
    }
    else if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::P ) { toggle_pause(); }
    }
  }

  process_move_keys();

  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::E ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ACTIVATE ) );
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::SWAP_INVENTORY ) );
  }
  if ( not imgui_wants_mouse() and sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
  {
    // get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::DIG ) );
  }
}

void SceneInputRouter::healing_spring_scene_state_handler()
{
  if ( is_paused() )
  {
    pause_overlay_input_handler();
    return;
  }
  if ( is_tooltip_paused() )
  {
    tooltip_pause_input_handler();
    return;
  }

  using namespace sf::Keyboard;
  // while the player stands at the spring/barber, the wheel picks the heal menu row and E buys it
  const bool heal_menu_open = ShopSystem::is_heal_menu_open( reg() );
  while ( const std::optional event = m_window.pollEvent() )
  {
    forward_to_imgui( *event );
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( is_mouse_event( *event ) and imgui_wants_mouse() ) continue;
    if ( heal_menu_open and process_heal_menu_wheel( *event ) ) continue;
    process_inventory_slot_input( *event, /*number_keys=*/true );
    if ( const auto *keyReleased = event->getIf<sf::Event::KeyReleased>() )
    {
      try_handle_debug_key( keyReleased->scancode );
      if ( keyReleased->scancode == sf::Keyboard::Scancode::Enter ) { get_systems_event_queue().trigger( Events::DismissTooltipEvent() ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Escape ) { queue_quit_game_event(); }
    }
    else if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::P ) { toggle_pause(); }
      // KeyPressed, not isKeyPressed: holding E buys only once
      else if ( heal_menu_open and keyPressed->scancode == sf::Keyboard::Scancode::E )
      {
        get_systems_event_queue().trigger( Events::OverlayEvent( Events::OverlayEvent::OverlayActions::HEAL_PURCHASE ) );
      }
    }
  }

  process_move_keys();

  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::E ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ACTIVATE ) );
    // get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::DROP_CARRYITEM ) );
  }
  if ( not imgui_wants_mouse() and sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ATTACK ) );
  }
}

void SceneInputRouter::shop_scene_state_handler()
{
  if ( is_paused() )
  {
    pause_overlay_input_handler();
    return;
  }
  if ( is_tooltip_paused() )
  {
    tooltip_pause_input_handler();
    return;
  }

  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    forward_to_imgui( *event );
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( is_mouse_event( *event ) and imgui_wants_mouse() ) continue;
    process_inventory_slot_input( *event, /*number_keys=*/false );
    if ( const auto *keyReleased = event->getIf<sf::Event::KeyReleased>() )
    {
      try_handle_debug_key( keyReleased->scancode );
      if ( keyReleased->scancode == sf::Keyboard::Scancode::Enter ) { get_systems_event_queue().trigger( Events::DismissTooltipEvent() ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Escape ) { queue_quit_game_event(); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Num1 ) { queue_buy_item_event( 1 ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Num2 ) { queue_buy_item_event( 2 ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Num3 ) { queue_buy_item_event( 3 ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Num4 ) { queue_buy_item_event( 4 ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Num5 ) { queue_buy_item_event( 5 ); }
    }
    else if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::P ) { toggle_pause(); }
    }
  }

  process_move_keys();

  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::E ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ACTIVATE ) );
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::SWAP_INVENTORY ) );
  }
  if ( not imgui_wants_mouse() and sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ATTACK ) );
  }
}

void SceneInputRouter::ruin_scene_state_handler()
{
  if ( is_paused() )
  {
    pause_overlay_input_handler();
    return;
  }
  if ( is_tooltip_paused() )
  {
    tooltip_pause_input_handler();
    return;
  }

  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    forward_to_imgui( *event );
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( is_mouse_event( *event ) and imgui_wants_mouse() ) continue;
    process_inventory_slot_input( *event, /*number_keys=*/true );
    if ( const auto *keyReleased = event->getIf<sf::Event::KeyReleased>() )
    {
      try_handle_debug_key( keyReleased->scancode );
      if ( keyReleased->scancode == sf::Keyboard::Scancode::Enter ) { get_systems_event_queue().trigger( Events::DismissTooltipEvent() ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Escape ) { queue_quit_game_event(); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Space )
      {
        get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::DESELECT_POSITION ) );
      }
    }
    else if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::P ) { toggle_pause(); }
    }
  }

  process_move_keys();

  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::E ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ACTIVATE ) );
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::SWAP_INVENTORY ) );
  }
  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Space ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::SELECT_POSITION ) );
  }
  if ( not imgui_wants_mouse() and sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ATTACK ) );
  }
}

void SceneInputRouter::game_over_scene_state_handler()
{
  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::R ) { enqueue( Events::SceneManagerEvent::Type::RETURN_TO_TITLE ); }
    }
  }
}

void SceneInputRouter::level_complete_scene_state_handler()
{
  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::R )
      {
        // the deepest level ends the run, otherwise the player restocks at the shop before the next level
        enqueue( Utils::Player::is_final_level_depth( reg() ) ? Events::SceneManagerEvent::Type::RETURN_TO_TITLE
                                                              : Events::SceneManagerEvent::Type::ENTER_SHOP );
      }
    }
  }
  // Even though there is no player movement in this scene, this function implicitly resets the Cmp::Direction.
  // This prevents bad direction data which can cause the player to wander off unexpectedly
  process_move_keys();
}

// PRIVATE

void SceneInputRouter::pause_overlay_input_handler()
{
  // gameplay is paused, so only the pause overlay's own controls get through. The grimoire and the F3 debug keys are
  // blocked: no other modal overlay or the debug panels may open while paused (see OverlayArbitrationSystem).
  while ( const std::optional event = m_window.pollEvent() )
  {
    // for the volume sliders
    forward_to_imgui( *event );
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::P ) { toggle_pause(); }
    }
  }
}

bool SceneInputRouter::is_paused() { return Sys::OverlayArbitrationSystem::active_modal( reg() ) == Sys::OverlayArbitrationSystem::Modal::PAUSEMENU; }

void SceneInputRouter::toggle_pause()
{
  get_systems_event_queue().trigger( Events::OverlayEvent( Events::OverlayEvent::OverlayActions::PAUSE_TOGGLE ) );
}

void SceneInputRouter::grimoire_overlay_input_handler()
{
  // gameplay is paused while the grimoire is open, so only the grimoire's own controls get through. P (pause) and the
  // F3 debug keys are blocked too: no other modal overlay or the debug panels may open while the grimoire is open
  // (see OverlayArbitrationSystem).
  while ( const std::optional event = m_window.pollEvent() )
  {
    // for the tab bar
    forward_to_imgui( *event );
    if ( dispatch_common_window_event( *event ) ) continue;
    process_grimoire_wheel( *event );
  }
  process_grimoire_keys();
}

void SceneInputRouter::tooltip_pause_input_handler()
{
  // gameplay is paused while a pausing tooltip is shown, so only Enter (dismiss) gets through. P (pause), the grimoire
  // and the F3 debug keys are blocked: no modal overlay or the debug panels may open during a tooltip pause (see
  // OverlayArbitrationSystem).
  while ( const std::optional event = m_window.pollEvent() )
  {
    forward_to_imgui( *event );
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( const auto *keyReleased = event->getIf<sf::Event::KeyReleased>() )
    {
      if ( keyReleased->scancode == sf::Keyboard::Scancode::Enter ) { get_systems_event_queue().trigger( Events::DismissTooltipEvent() ); }
    }
  }
}

bool SceneInputRouter::is_tooltip_paused()
{
  return Sys::OverlayArbitrationSystem::active_modal( reg() ) == Sys::OverlayArbitrationSystem::Modal::TOOLTIP;
}

bool SceneInputRouter::is_grimoire_open()
{
  return Sys::OverlayArbitrationSystem::active_modal( reg() ) == Sys::OverlayArbitrationSystem::Modal::GRIMOIRE;
}

void SceneInputRouter::process_grimoire_keys()
{
  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::G ) )
  {
    get_systems_event_queue().trigger( Events::OverlayEvent( Events::OverlayEvent::OverlayActions::GRIMOIRE_TOGGLE ) );
  }
  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Left ) )
  {
    get_systems_event_queue().trigger( Events::OverlayEvent( Events::OverlayEvent::OverlayActions::GRIMOIRE_PREV ) );
  }
  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Right ) )
  {
    get_systems_event_queue().trigger( Events::OverlayEvent( Events::OverlayEvent::OverlayActions::GRIMOIRE_NEXT ) );
  }
}

void SceneInputRouter::process_grimoire_wheel( const sf::Event &event )
{
  const auto *wheelScrolled = event.getIf<sf::Event::MouseWheelScrolled>();
  // scroll up/away for the previous grimoire page, down/towards for the next, like scrolling a document
  if ( wheelScrolled and wheelScrolled->wheel == sf::Mouse::Wheel::Vertical and wheelScrolled->delta != 0.f )
  {
    const auto action = wheelScrolled->delta > 0.f ? Events::OverlayEvent::OverlayActions::GRIMOIRE_NEXT
                                                   : Events::OverlayEvent::OverlayActions::GRIMOIRE_PREV;
    get_systems_event_queue().trigger( Events::OverlayEvent( action ) );
  }
}

bool SceneInputRouter::process_heal_menu_wheel( const sf::Event &event )
{
  const auto *wheelScrolled = event.getIf<sf::Event::MouseWheelScrolled>();
  if ( not wheelScrolled or wheelScrolled->wheel != sf::Mouse::Wheel::Vertical ) return false;
  // swallow the wheel either way, so it never changes the inventory slot while the heal menu is open
  if ( wheelScrolled->delta == 0.f ) return true;
  // scroll up/away for the previous row, down/towards for the next, like the inventory slots
  const auto action = wheelScrolled->delta > 0.f ? Events::OverlayEvent::OverlayActions::HEAL_PREV : Events::OverlayEvent::OverlayActions::HEAL_NEXT;
  get_systems_event_queue().trigger( Events::OverlayEvent( action ) );
  return true;
}

void SceneInputRouter::process_inventory_slot_input( const sf::Event &event, bool number_keys )
{
  using Mode = Events::SelectInventorySlotEvent::Mode;
  if ( const auto *wheelScrolled = event.getIf<sf::Event::MouseWheelScrolled>() )
  {
    // scroll up/away for the previous slot, down/towards for the next
    if ( wheelScrolled->wheel != sf::Mouse::Wheel::Vertical or wheelScrolled->delta == 0.f ) return;
    get_systems_event_queue().trigger( Events::SelectInventorySlotEvent( wheelScrolled->delta > 0.f ? Mode::PREV : Mode::NEXT ) );
    return;
  }
  if ( not number_keys ) return;
  if ( const auto *keyPressed = event.getIf<sf::Event::KeyPressed>() )
  {
    // the number row only: scancodes never match the numpad keys, which are debug keys
    using sf::Keyboard::Scancode;
    static constexpr std::array<Scancode, Cmp::kMaxInventorySlots> kSlotKeys{ Scancode::Num1, Scancode::Num2, Scancode::Num3, Scancode::Num4,
                                                                              Scancode::Num5 };
    const auto it = std::ranges::find( kSlotKeys, keyPressed->scancode );
    if ( it == kSlotKeys.end() ) return;
    const auto idx = static_cast<uint8_t>( std::distance( kSlotKeys.begin(), it ) );
    get_systems_event_queue().trigger( Events::SelectInventorySlotEvent( Mode::INDEX, idx ) );
  }
}

void SceneInputRouter::process_move_keys()
{
  // allow multiple changes to the direction vector, otherwise we get a delayed slurred movement
  auto player_direction_view = reg().view<Cmp::Player::Character, Cmp::Direction>();
  for ( auto [entity, player, direction] : player_direction_view.each() )
  {
    direction.x = 0;
    direction.y = 0;
    if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::W ) ) { direction.y = -1.0f; } // move player up
    if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::A ) ) { direction.x = -1.0f; } // move player left
    if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::D ) ) { direction.x = 1.0f; }  // move player right
    if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::S ) ) { direction.y = 1.0f; }  // move player down
  }
}

void SceneInputRouter::forward_to_imgui( const sf::Event &event )
{
  // OverlayArbitrationSystem pumps an ImGui frame every gameplay frame, so forwarded events never pile up in ImGui's
  // queue (see comment in settings_scene_state_handler).
  ImGui::SFML::ProcessEvent( m_window, event );
}

bool SceneInputRouter::imgui_wants_mouse()
{
  // WantCaptureMouse is left over from the last ImGui frame, i.e. whether the mouse is over (or dragging) an overlay
  // window that takes input, such as the F3 debug panels. The grimoire hover overlay never takes input.
  return ImGui::GetIO().WantCaptureMouse;
}

bool SceneInputRouter::is_mouse_event( const sf::Event &event )
{
  return event.is<sf::Event::MouseButtonPressed>() or event.is<sf::Event::MouseButtonReleased>() or event.is<sf::Event::MouseWheelScrolled>();
}

bool SceneInputRouter::dispatch_common_window_event( const sf::Event &event )
{
  if ( event.is<sf::Event::Closed>() )
  {
    m_window.close();
    return true;
  }
  if ( const auto *resized = event.getIf<sf::Event::Resized>() )
  {
    resize_window( resized->size );
    return true;
  }
  return false;
}

template <typename SettingComponent>
void SceneInputRouter::toggle_setting( std::string_view log_prefix )
{
  bool &current_setting = Utils::scene_setting<SettingComponent>( reg() ).enabled;
  current_setting = not current_setting;
  SPDLOG_INFO( "{} now {}", log_prefix, current_setting ? "ENABLED" : "DISABLED" );
}

void SceneInputRouter::try_handle_debug_key( [[maybe_unused]] sf::Keyboard::Scancode scancode )
{
#ifndef NDEBUG
  using namespace sf::Keyboard;
  if ( scancode == Scancode::F1 ) { toggle_setting<Cmp::SceneSettings::CollisionDetection>( "Collisions are" ); }
  else if ( scancode == Scancode::F2 ) { toggle_setting<Cmp::SceneSettings::ShowPathFinding>( "Show pathfinding is" ); }
  else if ( scancode == Scancode::F3 )
  {
    get_systems_event_queue().trigger( Events::OverlayEvent( Events::OverlayEvent::OverlayActions::DEBUG_TOGGLE ) );
  }
  else if ( scancode == Scancode::F4 ) { toggle_setting<Cmp::SceneSettings::ShowNavmesh>( "Show navmesh is" ); }
  else if ( scancode == Scancode::F9 ) { toggle_setting<Cmp::SceneSettings::Shaders>( "Shaders are" ); }
  else if ( scancode == Scancode::F10 ) { toggle_setting<Cmp::SceneSettings::Footsteps>( "Footsteps are" ); }
  else if ( scancode == Scancode::F11 ) { queue_suicide_event(); }
  else if ( scancode == Scancode::Numpad3 ) { Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, Cmp::Stats::Infamy{ 10 }, {}, {}, {} } ); }
  else if ( scancode == Scancode::Numpad2 ) { Utils::Player::get_stats( reg() ).apply( { {}, {}, Cmp::Stats::Despair{ 10 }, {}, {}, {}, {} } ); }
  else if ( scancode == Scancode::NumpadEnter ) { Utils::Player::get_stats( reg() ).apply( { Cmp::Stats::Health{ 10 }, {}, {}, {}, {}, {}, {} } ); }
  else if ( scancode == Scancode::NumpadMinus ) { Utils::Player::get_stats( reg() ).decay_all_toxidrome( 10 ); }
  else if ( scancode == Scancode::NumpadPlus )
  {
    auto brady = Factory::Toxicity::create_toxidrome( "bradycardia", 10 );
    Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, {}, {}, {}, brady } );
    auto venom = Factory::Toxicity::create_toxidrome( "venom", 10 );
    Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, {}, {}, {}, venom } );
    auto photo = Factory::Toxicity::create_toxidrome( "phototoxia", 10 );
    Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, {}, {}, {}, photo } );
    auto hallucin = Factory::Toxicity::create_toxidrome( "halucinogen", 10 );
    Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, {}, {}, {}, hallucin } );
    auto hypoxia = Factory::Toxicity::create_toxidrome( "hypoxia", 10 );
    Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, {}, {}, {}, hypoxia } );
    auto vertigo = Factory::Toxicity::create_toxidrome( "vertigo", 10 );
    Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, {}, {}, {}, vertigo } );
  }
  else if ( scancode == Scancode::NumpadMultiply ) { Utils::Player::get_wealth( reg() ).wealth += 1; }
  else if ( scancode == Scancode::NumpadDivide )
  {
    auto &cadaver_cmp = Utils::Player::get_cadaver( reg() );
    auto pool = cadaver_cmp.keys( /*remaining_only=*/true );
    auto pick = pool.at( Cmp::RandomInt( 0, pool.size() - 1 ).gen() );
    cadaver_cmp.set( pick );
  }
  else if ( scancode == Scancode::Numpad7 ) { Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, Cmp::Stats::Luck{ 10 }, {}, {} } ); }
  else if ( scancode == Scancode::Numpad4 ) { Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, Cmp::Stats::Luck{ -10 }, {}, {} } ); }
  else if ( scancode == Scancode::Numpad9 ) { Utils::Player::get_stats( reg() ).apply( { {}, Cmp::Stats::Fear{ 10 }, {}, {}, {}, {}, {} } ); }
  else if ( scancode == Scancode::Numpad6 ) { Utils::Player::get_stats( reg() ).apply( { {}, Cmp::Stats::Fear{ -10 }, {}, {}, {}, {}, {} } ); }
  else if ( scancode == Scancode::Numpad8 ) { Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, {}, Cmp::Stats::Anima{ 10 }, {} } ); }
  else if ( scancode == Scancode::Numpad5 ) { Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, {}, Cmp::Stats::Anima{ -10 }, {} } ); }
  else if ( scancode == Scancode::F5 )
  {
    Factory::Player::add_player_last_graveyard_pos( reg(), Utils::Player::get_position( reg() ), { 0.f, 0.f } );
    enqueue( Events::SceneManagerEvent::Type::ENTER_CRYPT );
  }
  else if ( scancode == Scancode::F6 )
  {
    Factory::Player::add_player_last_graveyard_pos( reg(), Utils::Player::get_position( reg() ), { 0.f, 0.f } );
    enqueue( Events::SceneManagerEvent::Type::ENTER_SACREDSPRING );
  }
  else if ( scancode == Scancode::F7 )
  {
    Factory::Player::add_player_last_graveyard_pos( reg(), Utils::Player::get_position( reg() ), { 0.f, 0.f } );
    enqueue( Events::SceneManagerEvent::Type::ENTER_RUIN_LOWER );
  }
  else if ( scancode == Scancode::F8 )
  {
    // skipping the exit means the exit key is never consumed by unlocking it (see ExitSystem); drop it here so it
    // doesn't follow the player into the next scene and keep the compass's exit needle alive
    Factory::Player::destroy_inventory( reg(), "item.exitkey" );
    enqueue( Events::SceneManagerEvent::Type::LEVEL_COMPLETE );
    Factory::Player::remove_player_last_graveyard_pos( reg() );
  }
  else if ( scancode == Scancode::F12 ) { get_systems_event_queue().trigger( Events::LightningEvent( Utils::Player::get_entity( reg() ) ) ); }
#endif // NDEBUG
}

void SceneInputRouter::resize_window( sf::Vector2u size )
{
  sf::FloatRect visibleArea( { 0.f, 0.f }, sf::Vector2f( size ) );
  m_window.setView( sf::View( visibleArea ) );
}

void SceneInputRouter::queue_suicide_event()
{
  get_systems_event_queue().enqueue( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::SUICIDE, Utils::Player::get_position( reg() ) ) );
}

void SceneInputRouter::queue_buy_item_event( uint8_t item_idx ) { get_systems_event_queue().enqueue( Events::BuyShopItemEvent( item_idx ) ); }

void SceneInputRouter::queue_quit_game_event()
{
  // Prevent skipping the death animation and leaving the game in a bad state
  if ( Utils::Player::get_mortality( reg() ).state != Cmp::Player::Mortality::State::ALIVE ) return;
  m_sound_bank.stop_all();
  enqueue( Events::SceneManagerEvent::Type::QUIT_GAME );
}

void SceneInputRouter::enqueue( Events::SceneManagerEvent::Type type )
{
  m_scenemanager_event_dispatcher.enqueue( Events::SceneManagerEvent( type ) );
}

void SceneInputRouter::toggle_particle_test( bool enable )
{
  for ( auto [entt, owner] : reg().view<Cmp::Particle::SpriteOwner>().each() )
  {
    if ( enable )
    {
      owner.sprite->restart();
      SPDLOG_INFO( "Particle sprite {} is now ENABLED", owner.sprite->get_tag() );
    }
    else
    {
      owner.sprite->stop();
      SPDLOG_INFO( "Particle sprite {} is now DISABLED", owner.sprite->get_tag() );
    }
  }
}

} // namespace Game::Sys