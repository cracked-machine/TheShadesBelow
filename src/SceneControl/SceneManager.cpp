#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Ruin/ObjectiveType.hpp>
#include <Factory/CryptFactory.hpp>
#include <SceneControl/IScene.hpp>
#include <SceneControl/RegistryTransfer.hpp>
#include <SceneControl/Scene.hpp>
#include <SceneControl/SceneManager.hpp>
#include <SceneControl/Scenes/CryptScene.hpp>
#include <SceneControl/Scenes/GameOverScene.hpp>
#include <SceneControl/Scenes/GraveyardScene.hpp>
#include <SceneControl/Scenes/HealingSpringScene.hpp>
#include <SceneControl/Scenes/LevelCompleteScene.hpp>
#include <SceneControl/Scenes/PausedMenuScene.hpp>
#include <SceneControl/Scenes/RuinSceneLowerFloor.hpp>
#include <SceneControl/Scenes/RuinSceneUpperFloor.hpp>
#include <SceneControl/Scenes/SettingsMenuScene.hpp>
#include <SceneControl/Scenes/ShopScene.hpp>
#include <SceneControl/Scenes/TitleScene.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/SpriteStore.hpp>

namespace Game::Scene
{

SceneManager::SceneManager( sf::RenderWindow &w, Audio::SoundBank &sound_bank, Sys::Store &system_store, entt::dispatcher &nav_event_dispatcher,
                            entt::dispatcher &scenemanager_event_dispatcher )
    : m_window( w ),
      m_sound_bank( sound_bank ),
      m_system_store( system_store ),
      m_nav_event_dispatcher( nav_event_dispatcher ),
      m_scenemanager_event_dispatcher( scenemanager_event_dispatcher )
{
  m_scenemanager_event_dispatcher.sink<Events::SceneManagerEvent>().connect<&SceneManager::handle_events>( this );
}

void SceneManager::update( sf::Time dt )
{

  if ( m_scene_stack.empty() ) return;

  // the manager begins and presents the frame; the current scene draws into it
  m_window.clear();
  m_scene_stack.current().update( dt );
  m_window.display();

  // Now safely consume any deferred events outside of Scene::update()
  if ( m_nav_event_dispatcher.size() > 0 ) { m_nav_event_dispatcher.update(); }
  if ( m_scenemanager_event_dispatcher.size() > 0 ) { m_scenemanager_event_dispatcher.update(); }
}

void SceneManager::push( std::unique_ptr<IScene> new_scene, RegCopyMode mode )
{
  RegistryTransfer::RegCopy reg_copy = nullptr;

  // in case this is the first scene pushed, avoid using empty stack front
  if ( not m_scene_stack.empty() )
  {
    reg_copy = m_reg_xfer.copy_reg( m_scene_stack.current(), mode );
    loading_screen( [&]() { m_scene_stack.current().on_exit(); }, m_splash_texture );
  }

  m_scene_stack.push( std::move( new_scene ) );
  m_scene_stack.print_stack();

  inject_current_scene_registry_into_systems();

  if ( reg_copy && mode == RegCopyMode::ALL )
  {
    m_reg_xfer.xfer_player_entt( *reg_copy, m_scene_stack.current().registry() );
    m_reg_xfer.xfer_component_entt<Cmp::PlayerInventorySlot>( *reg_copy, m_scene_stack.current().registry() );
    m_reg_xfer.xfer_component_entt<Cmp::Ruin::ObjectiveType>( *reg_copy, m_scene_stack.current().registry() );
  }

  loading_screen( [&]() { m_scene_stack.current().on_init(); }, m_splash_texture );
  loading_screen( [&]() { m_scene_stack.current().on_enter(); }, m_splash_texture );
}

void SceneManager::push_no_exit( std::unique_ptr<IScene> new_scene, RegCopyMode mode )
{
  RegistryTransfer::RegCopy reg_copy = nullptr;

  // in case this is the first scene pushed, avoid using empty stack front
  if ( not m_scene_stack.empty() ) { reg_copy = m_reg_xfer.copy_reg( m_scene_stack.current(), mode ); }

  m_scene_stack.push( std::move( new_scene ) );
  m_scene_stack.print_stack();

  inject_current_scene_registry_into_systems();

  if ( reg_copy && mode == RegCopyMode::ALL )
  {
    m_reg_xfer.xfer_player_entt( *reg_copy, m_scene_stack.current().registry() );
    m_reg_xfer.xfer_component_entt<Cmp::PlayerInventorySlot>( *reg_copy, m_scene_stack.current().registry() );
    m_reg_xfer.xfer_component_entt<Cmp::Ruin::ObjectiveType>( *reg_copy, m_scene_stack.current().registry() );
  }

  loading_screen( [&]() { m_scene_stack.current().on_init(); }, m_splash_texture );
  loading_screen( [&]() { m_scene_stack.current().on_enter(); }, m_splash_texture );
}

void SceneManager::pop( RegCopyMode mode )
{
  if ( m_scene_stack.size() == 1 ) return;

  RegistryTransfer::RegCopy reg_copy = m_reg_xfer.copy_reg( m_scene_stack.current(), mode );
  loading_screen( [&]() { m_scene_stack.current().on_exit(); }, m_splash_texture );

  m_scene_stack.pop();
  m_scene_stack.print_stack();

  inject_current_scene_registry_into_systems();

  if ( reg_copy && mode == RegCopyMode::ALL )
  {
    m_reg_xfer.xfer_player_entt( *reg_copy, m_scene_stack.current().registry() );
    m_reg_xfer.xfer_component_entt<Cmp::PlayerInventorySlot>( *reg_copy, m_scene_stack.current().registry() );
    m_reg_xfer.xfer_component_entt<Cmp::Ruin::ObjectiveType>( *reg_copy, m_scene_stack.current().registry() );
  }

  if ( !m_scene_stack.empty() )
  {
    loading_screen( [&]() { m_scene_stack.current().on_enter(); }, m_splash_texture );
  }
}

void SceneManager::pop_no_exit( RegCopyMode mode )
{
  if ( m_scene_stack.size() == 1 ) return;

  RegistryTransfer::RegCopy reg_copy = m_reg_xfer.copy_reg( m_scene_stack.current(), mode );

  m_scene_stack.pop();
  m_scene_stack.print_stack();

  inject_current_scene_registry_into_systems();

  if ( reg_copy && mode == RegCopyMode::ALL )
  {
    m_reg_xfer.xfer_player_entt( *reg_copy, m_scene_stack.current().registry() );
    m_reg_xfer.xfer_component_entt<Cmp::PlayerInventorySlot>( *reg_copy, m_scene_stack.current().registry() );
    m_reg_xfer.xfer_component_entt<Cmp::Ruin::ObjectiveType>( *reg_copy, m_scene_stack.current().registry() );
  }

  loading_screen( [&]() { m_scene_stack.current().on_enter(); }, m_splash_texture );
}

void SceneManager::replace( std::unique_ptr<IScene> new_scene, RegCopyMode mode )
{
  RegistryTransfer::RegCopy reg_copy = m_reg_xfer.copy_reg( m_scene_stack.current(), mode );

  // exit the current scene before popping
  loading_screen( [&]() { m_scene_stack.current().on_exit(); }, m_splash_texture );

  // replace the current scene
  m_scene_stack.print_stack();
  SPDLOG_DEBUG( "Replacing {}.", m_scene_stack.current().get_name() );
  m_scene_stack.pop();
  m_scene_stack.push( std::move( new_scene ) );
  m_scene_stack.print_stack();

  inject_current_scene_registry_into_systems();

  if ( reg_copy && mode == RegCopyMode::ALL )
  {
    m_reg_xfer.xfer_player_entt( *reg_copy, m_scene_stack.current().registry() );
    m_reg_xfer.xfer_component_entt<Cmp::PlayerInventorySlot>( *reg_copy, m_scene_stack.current().registry() );
    m_reg_xfer.xfer_component_entt<Cmp::Ruin::ObjectiveType>( *reg_copy, m_scene_stack.current().registry() );
  }

  loading_screen( [&]() { m_scene_stack.current().on_init(); }, m_splash_texture );
  loading_screen( [&]() { m_scene_stack.current().on_enter(); }, m_splash_texture );
}

void SceneManager::replace_no_exit( std::unique_ptr<IScene> new_scene, RegCopyMode mode )
{
  RegistryTransfer::RegCopy reg_copy = m_reg_xfer.copy_reg( m_scene_stack.current(), mode );

  // replace the current scene without exiting it
  m_scene_stack.print_stack();
  SPDLOG_DEBUG( "Replacing {} (overlay).", m_scene_stack.current().get_name() );
  m_scene_stack.pop();
  m_scene_stack.push( std::move( new_scene ) );
  m_scene_stack.print_stack();

  inject_current_scene_registry_into_systems();

  if ( reg_copy && mode == RegCopyMode::ALL )
  {
    m_reg_xfer.xfer_player_entt( *reg_copy, m_scene_stack.current().registry() );
    m_reg_xfer.xfer_component_entt<Cmp::PlayerInventorySlot>( *reg_copy, m_scene_stack.current().registry() );
    m_reg_xfer.xfer_component_entt<Cmp::Ruin::ObjectiveType>( *reg_copy, m_scene_stack.current().registry() );
  }

  loading_screen( [&]() { m_scene_stack.current().on_init(); }, m_splash_texture );
  loading_screen( [&]() { m_scene_stack.current().on_enter(); }, m_splash_texture );
}

void SceneManager::handle_events( const Events::SceneManagerEvent &event )
{
  switch ( event.m_type )
  {
    case Events::SceneManagerEvent::Type::START_GAME: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::START_GAME requested" );
      auto graveyard_scene = std::make_unique<GraveyardScene>( m_sound_bank, m_system_store, m_nav_event_dispatcher );
      push( std::move( graveyard_scene ) );
      break;
    }
    case Events::SceneManagerEvent::Type::ENTER_CRYPT: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::ENTER_CRYPT requested" );
      auto crypt_scene = std::make_unique<CryptScene>( m_sound_bank, m_system_store, m_nav_event_dispatcher );
      push_no_exit( std::move( crypt_scene ), RegCopyMode::ALL );
      break;
    }
    case Events::SceneManagerEvent::Type::EXIT_CRYPT: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::EXIT_CRYPT requested" );
      pop( RegCopyMode::ALL );
      break;
    }
    case Events::SceneManagerEvent::Type::ENTER_SACREDSPRING: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::ENTER_SACREDSPRING requested" );
      auto healing_spring_scene = std::make_unique<HealingSpringScene>( m_sound_bank, m_system_store, m_nav_event_dispatcher );
      push_no_exit( std::move( healing_spring_scene ), RegCopyMode::ALL );
      break;
    }
    case Events::SceneManagerEvent::Type::EXIT_SACREDSPRING: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::EXIT_SACREDSPRING requested" );
      pop( RegCopyMode::ALL );
      break;
    }
    case Events::SceneManagerEvent::Type::ENTER_SHOP: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::ENTER_SHOP requested" );
      auto shop_scene = std::make_unique<ShopScene>( m_sound_bank, m_system_store, m_nav_event_dispatcher );
      replace( std::move( shop_scene ), RegCopyMode::ALL );
      break;
    }
    case Events::SceneManagerEvent::Type::EXIT_SHOP: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::EXIT_SHOP requested" );
      auto graveyard_scene = std::make_unique<GraveyardScene>( m_sound_bank, m_system_store, m_nav_event_dispatcher );
      replace( std::move( graveyard_scene ), RegCopyMode::ALL );
      break;
    }
    case Events::SceneManagerEvent::Type::ENTER_RUIN_LOWER: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::ENTER_RUIN_LOWER requested" );
      auto ruin_scene = std::make_unique<RuinSceneLowerFloor>( m_sound_bank, m_system_store, m_nav_event_dispatcher );
      ruin_scene->set_entry_mode( RuinSceneLowerFloor::EntryMode::FROM_DOOR );
      push_no_exit( std::move( ruin_scene ), RegCopyMode::ALL );
      break;
    }
    case Events::SceneManagerEvent::Type::ENTER_RUIN_UPPER: {
      if ( m_scene_stack.current().get_name() == "RuinSceneUpperFloor" )
      {
        SPDLOG_WARN( "Already in RuinSceneUpperFloor, ignoring duplicate ENTER_RUIN_UPPER" );
        break;
      }
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::ENTER_RUIN_UPPER requested" );
      auto ruin_scene = std::make_unique<RuinSceneUpperFloor>( m_sound_bank, m_system_store, m_nav_event_dispatcher );
      push_no_exit( std::move( ruin_scene ), RegCopyMode::ALL );
      break;
    }
    case Events::SceneManagerEvent::Type::EXIT_RUIN_UPPER: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::EXIT_RUIN_UPPER requested" );
      static_cast<RuinSceneLowerFloor *>( m_scene_stack.previous() )->set_entry_mode( RuinSceneLowerFloor::EntryMode::FROM_UPPER_FLOOR );
      pop( RegCopyMode::ALL );
      break;
    }
    case Events::SceneManagerEvent::Type::EXIT_RUIN: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::EXIT_RUIN requested" );
      pop( RegCopyMode::ALL );
      break;
    }
    case Events::SceneManagerEvent::Type::EXIT_GAME: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::EXIT_GAME requested" );
      m_window.close();
      break;
    }
    case Events::SceneManagerEvent::Type::SETTINGS_MENU: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::SETTINGS_MENU requested" );
      auto settings_scene = std::make_unique<SettingsMenuScene>( m_system_store, m_nav_event_dispatcher );
      push( std::move( settings_scene ) );
      break;
    }
    case Events::SceneManagerEvent::Type::EXIT_SETTINGS_MENU: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::EXIT_SETTINGS_MENU requested" );
      pop();
      break;
    }
    case Events::SceneManagerEvent::Type::QUIT_GAME: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::QUIT_GAME requested" );
      pop();
      break;
    }
    case Events::SceneManagerEvent::Type::PAUSE_GAME: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::PAUSE_GAME requested" );
      auto paused_scene = std::make_unique<PausedMenuScene>( m_sound_bank, m_system_store, m_nav_event_dispatcher );
      push_no_exit( std::move( paused_scene ) );
      break;
    }
    case Events::SceneManagerEvent::Type::RESUME_GAME: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::RESUME_GAME requested" );
      pop_no_exit();
      break;
    }
    case Events::SceneManagerEvent::Type::GAME_OVER: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::GAME_OVER requested" );
      auto game_over_scene = std::make_unique<GameOverScene>( m_sound_bank, m_system_store, m_nav_event_dispatcher );
      push_no_exit( std::move( game_over_scene ), RegCopyMode::ALL );
      break;
    }
    case Events::SceneManagerEvent::Type::LEVEL_COMPLETE: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::LEVEL_COMPLETE requested" );
      auto level_complete_scene = std::make_unique<LevelCompleteScene>( m_sound_bank, m_system_store, m_nav_event_dispatcher );
      replace( std::move( level_complete_scene ), RegCopyMode::ALL );
      break;
    }
    case Events::SceneManagerEvent::Type::RETURN_TO_TITLE: {
      SPDLOG_DEBUG( "SceneManager: Events::SceneManagerEvent::Type::RETURN_TO_TITLE requested" );
      // assume last scene is always title scene
      while ( m_scene_stack.size() > 1 )
      {
        pop();
      }
      break;
    }
    default:
      break;
  }
}

// PRIVATE FUNCTIONS

void SceneManager::inject_current_scene_registry_into_systems()
{
  if ( m_scene_stack.empty() ) { throw std::runtime_error( "SceneManager::inject_registry: No current scene available" ); }
  entt::registry &reg = m_scene_stack.current().registry();
  for ( auto &sys : m_system_store )
    sys.second->reg( reg ); // pass the unique_ptr by reference

  SPDLOG_DEBUG( "Injected registry into {} systems for {}", m_system_store.size(), m_scene_stack.current().get_name() );
}

} // namespace Game::Scene