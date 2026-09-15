#include <Audio/SoundBank.hpp>
#include <Components/Direction.hpp>
#include <Components/Exit.hpp>
#include <Components/Persistent/ExitKeyRequirement.hpp>
#include <Components/Persistent/MaxNumAltars.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/BlastRadius.hpp>
#include <Components/Player/CadaverCount.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/EatingTimeAccumulator.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/SceneSettings/Footsteps.hpp>
#include <Components/SceneSettings/Shaders.hpp>
#include <Components/SceneSettings/ShowDebugStats.hpp>
#include <Components/SceneSettings/ShowNavmesh.hpp>
#include <Components/SceneSettings/ShowPathFinding.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Events/BuyShopItemEvent.hpp>
#include <Events/CryptRoomEvent.hpp>
#include <Events/DropInventoryEvent.hpp>
#include <Events/LightningEvent.hpp>
#include <Events/LoadSettingsEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Events/SaveSettingsEvent.hpp>
#include <Events/UnlockDoorEvent.hpp>
#include <Factory/ActionFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SceneControl/Events/ProcessCryptSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessGameoverSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessGraveyardSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessHealingSpringSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessLevelCompleteSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessPausedMenuSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessRuinSceneLowerInputEvent.hpp>
#include <SceneControl/Events/ProcessRuinSceneUpperInputEvent.hpp>
#include <SceneControl/Events/ProcessSettingsMenuSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessShopSceneInputEvent.hpp>
#include <SceneControl/Events/ProcessTitleSceneInputEvent.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <SceneControl/SceneInputRouter.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <imgui-SFML.h>

namespace Game::Sys
{

SceneInputRouter::SceneInputRouter( entt::registry &reg, sf::RenderWindow &m_window, Sprites::SpriteFactory &sprite_factory,
                                    Audio::SoundBank &sound_bank, entt::dispatcher &nav_event_dispatcher,
                                    entt::dispatcher &scenemanager_event_dispatcher )
    : Sys::BaseSystem( reg, m_window, sprite_factory, sound_bank ),
      m_nav_event_dispatcher( nav_event_dispatcher ),
      m_scenemanager_event_dispatcher( scenemanager_event_dispatcher )
{
  // clang-format off
  m_nav_event_dispatcher.sink<Events::ProcessTitleSceneInputEvent>().connect<&SceneInputRouter::title_scene_input_handler>( this );
  m_nav_event_dispatcher.sink<Events::ProcessSettingsMenuSceneInputEvent>().connect<&SceneInputRouter::settings_scene_state_handler>( this );
  m_nav_event_dispatcher.sink<Events::ProcessGraveyardSceneInputEvent>().connect<&SceneInputRouter::graveyard_scene_state_handler>(this );
  m_nav_event_dispatcher.sink<Events::ProcessPausedMenuSceneInputEvent>().connect<&SceneInputRouter::paused_scene_state_handler>(this );
  m_nav_event_dispatcher.sink<Events::ProcessGameoverSceneInputEvent>().connect<&SceneInputRouter::game_over_scene_state_handler>(this );
  m_nav_event_dispatcher.sink<Events::ProcessLevelCompleteSceneInputEvent>().connect<&SceneInputRouter::level_complete_scene_state_handler>( this );
  m_nav_event_dispatcher.sink<Events::ProcessCryptSceneInputEvent>().connect<&SceneInputRouter::crypt_scene_state_handler>( this );
  m_nav_event_dispatcher.sink<Events::ProcessHealingSpringSceneInputEvent>().connect<&SceneInputRouter::healing_spring_scene_state_handler>( this );
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

  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( const auto *keyReleased = event->getIf<sf::Event::KeyReleased>() )
    {
      if ( try_handle_debug_key( keyReleased->scancode ) ) {}
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::F5 )
      {
        Factory::Player::add_player_last_graveyard_pos( reg(), Utils::Player::get_position( reg() ), { 0.f, 0.f } );

        enqueue( Events::SceneManagerEvent::Type::ENTER_CRYPT );
      }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::F6 )
      {
        Factory::Player::add_player_last_graveyard_pos( reg(), Utils::Player::get_position( reg() ), { 0.f, 0.f } );

        enqueue( Events::SceneManagerEvent::Type::ENTER_SACREDSPRING );
      }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::F7 )
      {

        get_systems_event_queue().trigger( Events::DropInventoryEvent() );
        Factory::Player::add_player_last_graveyard_pos( reg(), Utils::Player::get_position( reg() ), { 0.f, 0.f } );

        enqueue( Events::SceneManagerEvent::Type::ENTER_RUIN_LOWER );
      }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::F8 ) { enqueue( Events::SceneManagerEvent::Type::ENTER_SHOP ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::F12 ) { get_systems_event_queue().trigger( Events::LightningEvent() ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Home ) { toggle_particle_test( true ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::End ) { toggle_particle_test( false ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Escape ) { queue_quit_game_event(); }
    }
    else if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::P ) { enqueue( Events::SceneManagerEvent::Type::PAUSE_GAME ); }
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
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::PLACE_BOMB ) );
  }
  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::G ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::TOGGLE_GRIMOIRE ) );
  }
  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::F ) ) { Factory::Action::try_eat_inventory( reg() ); }
  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::E ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ACTIVATE ) );
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::SWAP_INVENTORY ) );
  }
  if ( sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
  {
    // get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::DRAW_BOW ) );
    Factory::Action::try_burn_worlditem( reg() );
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ATTACK ) );
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::DIG ) );
  }
}

void SceneInputRouter::crypt_scene_state_handler()
{

  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( const auto *keyReleased = event->getIf<sf::Event::KeyReleased>() )
    {
      if ( try_handle_debug_key( keyReleased->scancode ) ) {}
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
      if ( keyPressed->scancode == sf::Keyboard::Scancode::P ) { enqueue( Events::SceneManagerEvent::Type::PAUSE_GAME ); }
    }
  }

  process_move_keys();

  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::E ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ACTIVATE ) );
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::SWAP_INVENTORY ) );
  }
  if ( sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
  {
    // get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::DIG ) );
  }
}

void SceneInputRouter::healing_spring_scene_state_handler()
{

  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( const auto *keyReleased = event->getIf<sf::Event::KeyReleased>() )
    {
      if ( try_handle_debug_key( keyReleased->scancode ) ) {}
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Escape ) { queue_quit_game_event(); }
    }
    else if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::P ) { enqueue( Events::SceneManagerEvent::Type::PAUSE_GAME ); }
    }
  }

  process_move_keys();

  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::E ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ACTIVATE ) );
    // get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::DROP_CARRYITEM ) );
  }
  if ( sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ATTACK ) );
  }
}

void SceneInputRouter::shop_scene_state_handler()
{

  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( const auto *keyReleased = event->getIf<sf::Event::KeyReleased>() )
    {
      if ( try_handle_debug_key( keyReleased->scancode ) ) {}
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Escape ) { queue_quit_game_event(); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Num1 ) { queue_buy_item_event( 1 ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Num2 ) { queue_buy_item_event( 2 ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Num3 ) { queue_buy_item_event( 3 ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Num4 ) { queue_buy_item_event( 4 ); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Num5 ) { queue_buy_item_event( 5 ); }
    }
    else if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::P ) { enqueue( Events::SceneManagerEvent::Type::PAUSE_GAME ); }
    }
  }

  process_move_keys();

  if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::E ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ACTIVATE ) );
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::SWAP_INVENTORY ) );
  }
  if ( sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ATTACK ) );
  }
}

void SceneInputRouter::ruin_scene_state_handler()
{

  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( const auto *keyReleased = event->getIf<sf::Event::KeyReleased>() )
    {
      if ( try_handle_debug_key( keyReleased->scancode ) ) {}
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Escape ) { queue_quit_game_event(); }
      else if ( keyReleased->scancode == sf::Keyboard::Scancode::Space )
      {
        get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::DESELECT_POSITION ) );
      }
    }
    else if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::P ) { enqueue( Events::SceneManagerEvent::Type::PAUSE_GAME ); }
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
  if ( sf::Mouse::isButtonPressed( sf::Mouse::Button::Left ) )
  {
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::ATTACK ) );
  }
}

void SceneInputRouter::paused_scene_state_handler()
{

  using namespace sf::Keyboard;
  while ( const std::optional event = m_window.pollEvent() )
  {
    // See comment in settings_scene_state_handler: only scenes that pump an ImGui frame
    // should forward events into ImGui's queue.
    ImGui::SFML::ProcessEvent( m_window, *event );
    if ( dispatch_common_window_event( *event ) ) continue;
    if ( const auto *keyPressed = event->getIf<sf::Event::KeyPressed>() )
    {
      if ( keyPressed->scancode == sf::Keyboard::Scancode::P ) { enqueue( Events::SceneManagerEvent::Type::RESUME_GAME ); }
    }
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
      if ( keyPressed->scancode == sf::Keyboard::Scancode::R ) { enqueue( Events::SceneManagerEvent::Type::ENTER_SHOP ); }
    }
  }
  // Even though there is no player movement in this scene, this function implicitly resets the Cmp::Direction.
  // This prevents bad direction data which can cause the player to wander off unexpectedly
  process_move_keys();
}

// PRIVATE

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

bool SceneInputRouter::try_handle_debug_key( sf::Keyboard::Scancode scancode )
{
  using namespace sf::Keyboard;
  if ( scancode == Scancode::F1 ) { toggle_setting<Cmp::SceneSettings::CollisionDetection>( "Collisions are" ); }
  else if ( scancode == Scancode::F2 ) { toggle_setting<Cmp::SceneSettings::ShowPathFinding>( "Show pathfinding is" ); }
  else if ( scancode == Scancode::F3 ) { toggle_setting<Cmp::SceneSettings::ShowDebugStats>( "Show debug stats is" ); }
  else if ( scancode == Scancode::F4 ) { toggle_setting<Cmp::SceneSettings::ShowNavmesh>( "Show navmesh is" ); }
  else if ( scancode == Scancode::F9 ) { toggle_setting<Cmp::SceneSettings::Shaders>( "Shaders are" ); }
  else if ( scancode == Scancode::F10 ) { toggle_setting<Cmp::SceneSettings::Footsteps>( "Footsteps are" ); }
  else if ( scancode == Scancode::F11 ) { queue_suicide_event(); }
  else if ( scancode == Scancode::Numpad1 ) { Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, {}, Cmp::Stats::Luck{ 10 }, {} } ); }
  else if ( scancode == Scancode::Numpad2 ) { Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, {}, Cmp::Stats::Luck{ -10 }, {} } ); }
  else if ( scancode == Scancode::Numpad3 ) { Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, Cmp::Stats::Infamy{ 10 }, {}, {}, {} } ); }
  else if ( scancode == Scancode::NumpadPlus ) { Utils::Player::get_stats( reg() ).apply( { Cmp::Stats::Health{ 10 }, {}, {}, {}, {}, {}, {} } ); }
  else if ( scancode == Scancode::Numpad4 ) { Utils::Player::get_wealth( reg() ).wealth += 1; }
  else if ( scancode == Scancode::Numpad5 ) { Utils::Player::get_cadaver_count( reg() ).increment_count( 1 ); }
  else if ( scancode == Scancode::Numpad7 ) { Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, Cmp::Stats::Toxicity{ -10 }, {}, {} } ); }
  else if ( scancode == Scancode::Numpad8 ) { Utils::Player::get_stats( reg() ).apply( { {}, {}, {}, {}, Cmp::Stats::Toxicity{ 10 }, {}, {} } ); }
  else if ( scancode == Scancode::Numpad9 ) { Utils::Player::get_stats( reg() ).apply( { {}, Cmp::Stats::Fear{ -10 }, {}, {}, {}, {}, {} } ); }
  else { return false; }
  return true;
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