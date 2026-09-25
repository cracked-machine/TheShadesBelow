#include <Components/Hazard/CorruptionCell.hpp>
#include <Components/Hazard/SinkholeCell.hpp>
#include <Components/Persistent/EffectsVolume.hpp>
#include <Components/Persistent/MusicVolume.hpp>
#include <Engine.hpp>
#include <Events/PauseClocksEvent.hpp>
#include <Events/ResumeClocksEvent.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <SceneControl/SceneManager.hpp>
#include <SceneControl/Scenes/CryptScene.hpp>
#include <SceneControl/Scenes/GraveyardScene.hpp>
#include <SceneControl/Scenes/TitleScene.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Systems/Threats/HazardFieldSystem.hpp>
#include <Systems/Threats/HazardFieldSystemImpl.hpp>
#include <Utils/ImGuiTheme.hpp>
#include <Utils/Utils.hpp>
#include <imgui-SFML.h>
#include <tracy/Tracy.hpp>

#include <memory>
#include <sstream>
#include <stacktrace>

namespace Game
{

Engine::Engine()
{

  if ( !m_window->isOpen() )
  {
    SPDLOG_CRITICAL( "Failed to create SFML RenderWindow" );
    std::terminate();
  }

  m_window->clear( sf::Color::Black );
  m_window->display();

  // setup ImGui here rather than RenderSystem classes to reduce white screen init time
  if ( not ImGui::SFML::Init( *m_window ) )
  {
    SPDLOG_CRITICAL( "ImGui-SFML initialization failed" );
    ImGui::SFML::Shutdown();
    std::terminate();
  }

  m_window->setVerticalSyncEnabled( true );
  // m_window->setFramerateLimit( 144 );

  // Set ImGui style
  Utils::apply_imgui_theme();
  ImGuiIO &io = ImGui::GetIO();
  io.FontGlobalScale = 1.5f;
  io.IniFilename = "res/imgui.ini";
  std::ignore = ImGui::SFML::UpdateFontTexture();

  // SPDLOG does not allow formatting raw pointers as strings for safety reasons.
  SPDLOG_INFO( "OpenGL: {}", reinterpret_cast<const char *>( glGetString( GL_VERSION ) ) );
  SPDLOG_INFO( "Renderer: {}", reinterpret_cast<const char *>( glGetString( GL_RENDERER ) ) );
  SPDLOG_INFO( "Vendor: {}", reinterpret_cast<const char *>( glGetString( GL_VENDOR ) ) );
}

bool Engine::run()
{
  try
  {
    loading_screen( [this]() { this->init_systems(); }, m_splash_texture );

    auto title_scene = std::make_unique<Scene::TitleScene>( *m_sound_bank, *m_system_store, m_nav_event_dispatcher );
    m_scene_manager->push( std::move( title_scene ) );

    sf::Clock tick;

    /// MAIN LOOP BEGINS
    while ( m_window->isOpen() )
    {
      sf::Time dt = tick.restart();

      // call the scene at the top of the scene manager stack
      m_scene_manager->update( dt );

      // trigger any enqueued events now we are outside of the scene update function.
      Sys::BaseSystem::get_systems_event_queue().update();

      // catch the resize events
      while ( const std::optional event = m_window->pollEvent() )
      {
        if ( const auto *resized = event->getIf<sf::Event::Resized>() )
        {
          // update the view to the new size of the window
          sf::FloatRect visibleArea( { 0.f, 0.f }, sf::Vector2f( resized->size ) );
          m_window->setView( sf::View( visibleArea ) );
        }
      }

      // marks the end of one presented frame for Tracy's frame-time graph; no-op unless
      // built with -DENABLE_TRACY=ON and a Tracy profiler is connected
      FrameMark;
    } /// MAIN LOOP ENDS

  } catch ( const std::exception &e )
  {
    SPDLOG_CRITICAL( "Unhandled exception in Engine::run(): {}", e.what() );
    show_error_screen( e.what() );
    return false;
  } catch ( ... )
  {
    SPDLOG_CRITICAL( "Unhandled unknown exception in Engine::run()" );
    show_error_screen( "An unknown error has occurred. Please check log." );
    return false;
  }
  return true; // exit game
}

void Engine::init_systems()
{
  m_sprite_factory = std::make_unique<Sprites::SpriteFactory>();
  m_sprite_factory->init();
  m_sound_bank->init();
  m_system_store = std::make_unique<Sys::Store>( *m_window, *m_sprite_factory, *m_sound_bank, m_nav_event_dispatcher, m_scenemanager_event_queue );
  m_scene_manager = std::make_unique<Scene::SceneManager>( *m_window, *m_sound_bank, *m_system_store, m_nav_event_dispatcher,
                                                           m_scenemanager_event_queue, *m_sprite_factory );

  SPDLOG_DEBUG( "Lazy initialization of systems complete" );
}

void Engine::show_error_screen( const std::string &error_msg )
{
  std::ostringstream stack_trace;
  stack_trace << std::stacktrace::current();
  SPDLOG_CRITICAL( "Stack trace:\n{}", stack_trace.str() );

  sf::Font font;
  if ( !font.openFromFile( "res/fonts/tuffy.ttf" ) )
  {
    // Can't even show error screen, just exit
    return;
  }

  // Show only the error message on screen, not the stack trace
  std::string screen_message = "Fatal Error\n\n" + error_msg + "\n\nPress any key to exit" + "\n\nFull stack trace saved to log.txt";

  static constexpr unsigned int kCharSize = 24;
  static constexpr float kScreenMargin = 50.f;
  const float max_text_width = static_cast<float>( m_window->getSize().x ) - ( kScreenMargin * 2.f );
  const float avg_char_width = font.getGlyph( 'M', kCharSize, false ).advance;
  const auto max_chars_per_line = static_cast<std::size_t>( max_text_width / avg_char_width );
  screen_message = Utils::wrap_text_to_width( screen_message, max_chars_per_line );

  sf::Text error_text( font, screen_message, kCharSize );
  error_text.setFillColor( sf::Color::White );
  error_text.setPosition( { kScreenMargin, kScreenMargin } );

  // The crash may have happened mid-render with the game's world camera view still active;
  // reset to the default view so the text's position maps to actual screen pixels.
  m_window->setView( m_window->getDefaultView() );

  m_window->clear( sf::Color::Black );
  m_window->draw( error_text );
  m_window->display();

  // Discard any events already queued before the error screen was shown (e.g. a movement key
  // still held down when the exception hit), so the wait loop below only reacts to input made
  // after the message is actually visible.
  while ( m_window->pollEvent() ) {}

  // Wait for key press
  while ( m_window->isOpen() )
  {
    while ( auto event = m_window->pollEvent() )
    {
      if ( event->is<sf::Event::Closed>() || event->is<sf::Event::KeyPressed>() )
      {
        ImGui::SFML::Shutdown();
        m_window->close();
        return;
      }
    }
  }
}

} // namespace Game