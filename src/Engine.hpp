#ifndef SRC_ENGINE_HPP__
#define SRC_ENGINE_HPP__

#include "imgui-SFML.h"
#include <SFML/Audio/Sound.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <entt/entity/fwd.hpp>

#include <Audio/SoundBank.hpp>
#include <Components/Font.hpp>
#include <SceneControl/SceneManager.hpp>
#include <Systems/Stores/SystemStore.hpp>

#include <future>
#include <memory>
#include <string>

namespace Game
{

//! @brief Top-level application engine: owns the render window, global subsystems, and the main
//! game loop tying everything together.
class Engine
{
public:
  //! @brief Construct the Engine: creates the render window, initializes ImGui-SFML, and logs GPU info.
  explicit Engine();

  //! @brief Engine is non-copyable.
  Engine( const Engine & ) = delete;
  //! @brief Engine is non-copyable.
  Engine &operator=( const Engine & ) = delete;
  //! @brief Engine is non-movable.
  Engine( Engine && ) = delete;
  //! @brief Engine is non-movable.
  Engine &operator=( Engine && ) = delete;

  //! @brief Runs the main game loop until the window is closed.
  //! @return bool True on normal exit; false if an unhandled exception terminated the loop early.
  bool run();

private:
  //! @brief Lazily initializes all ECS systems: the sprite factory, sound bank, system store,
  //! and scene manager.
  void init_systems();

  //! @brief Displays a fatal-error screen with `error_msg` and blocks until the user presses a
  //! key or closes the window.
  //! @param error_msg The error message to display.
  void show_error_screen( const std::string &error_msg );

  //! @brief Displays a loading screen with an animated "Loading..." caption while `callable` runs
  //! asynchronously, pumping window events so the app doesn't appear to hang.
  //! @tparam Callable Callable type invoked on a worker thread via std::async.
  //! @param callable The work to perform while the loading screen is shown.
  //! @param loading_texture Currently unused by the loading screen itself.
  template <typename Callable>
  void loading_screen( Callable &&callable, [[maybe_unused]] const sf::Texture &loading_texture )
  {
    Cmp::Font font( "res/fonts/tuffy.ttf" );
    sf::Text loading_text( font, "Loading", 48 );
    loading_text.setFillColor( sf::Color::White );
    // use fallback resolution for loading screen position since we dont have registry access at this point
    loading_text.setPosition( { ( Constants::kFallbackDisplaySize.x / 2.f ) - 50.f, ( Constants::kFallbackDisplaySize.y / 2.f ) + 100.f } );

    sf::Clock clock;
    const float text_update_interval = 1.f; // 1 second between dot updates
    int dot_count = 0;

    // Launch the initialization task asynchronously
    auto future = std::async( std::launch::async, std::forward<Callable>( callable ) );

    // Run code while waiting for completion
    while ( future.wait_for( std::chrono::milliseconds( 16 ) ) != std::future_status::ready )
    {
      // Handle window events to prevent "not responding"
      while ( const std::optional event = m_window->pollEvent() )
      {
        if ( event->is<sf::Event::Closed>() )
        {
          ImGui::SFML::Shutdown();
          m_window->close();
          return;
        }
      }

      // Update dots every second
      if ( clock.getElapsedTime().asSeconds() >= text_update_interval )
      {
        dot_count = ( dot_count + 1 ) % 4; // Cycle 0 -> 1 -> 2 -> 3 -> 0
        std::string dots( dot_count, '.' );
        loading_text.setString( "Loading" + dots );
        clock.restart();
      }

      m_window->clear( sf::Color::Black );
      m_window->draw( loading_text );
      m_window->display();
    }

    // Ensure any exceptions from the worker thread are propagated
    future.get();
  }

  //! @brief The splash texture, loaded first so it is ready before the loading screen needs it.
  sf::Texture m_splash_texture{ "res/textures/splash.png" };

  //! @brief The OpenGL render window.
  //! @note Uses a fallback resolution for the loading screen position since the registry (and its
  //! configured display size) is not yet accessible at this point.
  std::unique_ptr<sf::RenderWindow> m_window = std::make_unique<sf::RenderWindow>( sf::VideoMode( Constants::kFallbackDisplaySize ), "TheShadesBelow",
                                                                                   sf::State::Fullscreen );

  //! @brief Central sound effect/music playback manager.
  std::unique_ptr<Audio::SoundBank> m_sound_bank = std::make_unique<Audio::SoundBank>();

  //! @brief Owns and updates all ECS systems; created in init_systems().
  std::unique_ptr<Sys::Store> m_system_store;
  //! @brief Owns the scene stack and drives the active scene's update/render; created in init_systems().
  std::unique_ptr<Scene::SceneManager> m_scene_manager;

  //! @brief Dispatches navigation events (e.g. scene transition requests) between scenes and systems.
  entt::dispatcher m_nav_event_dispatcher;
  //! @brief Dispatches events consumed by the scene manager to coordinate scene lifecycle changes.
  entt::dispatcher m_scenemanager_event_queue;
};

} // namespace Game

#endif // SRC_ENGINE_HPP__
