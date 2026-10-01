#ifndef SRC_SCENECONTROL_SCENEMANAGER_HPP__
#define SRC_SCENECONTROL_SCENEMANAGER_HPP__

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <Audio/SoundBank.hpp>
#include <Components/Font.hpp>
#include <Components/Player/KeysCount.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <SceneControl/IScene.hpp>
#include <SceneControl/RegistryTransfer.hpp>
#include <SceneControl/SceneStack.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>

#include <Utils/Constants.hpp>
#include <future>
#include <memory>

namespace Game::Scene
{

//! @brief Owns the SceneStack and drives scene lifecycle transitions (push/pop/replace), showing a
//!        loading screen while each transition's on_init()/on_enter()/on_exit() callbacks run, and
//!        using RegistryTransfer to carry player/component state between the outgoing and incoming
//!        scene's registries. Also listens for Events::SceneManagerEvent to trigger transitions.
class SceneManager
{
public:
  //! @brief Construct a new Scene Manager object
  //! @param w The OpenGL render window
  //! @param sound_bank Sound bank used by scenes for playback
  //! @param system_store Store of all game systems; the active scene's registry is injected into it on transition
  //! @param nav_event_dispatcher Dispatcher used to route per-scene input-processing events
  //! @param scenemanager_event_dispatcher Dispatcher used to receive scene transition requests
  explicit SceneManager( sf::RenderWindow &w, Audio::SoundBank &sound_bank, Sys::Store &system_store, entt::dispatcher &nav_event_dispatcher,
                         entt::dispatcher &scenemanager_event_dispatcher );

  // Prevent copy/move of SceneManager object. Use unique ptr.
  SceneManager( const SceneManager &other ) = delete;
  SceneManager &operator=( const SceneManager & ) = delete;
  SceneManager( SceneManager &&other ) = delete;
  SceneManager &operator=( SceneManager && ) = delete;

  //! @brief Update the current scene
  //! @param dt The time elapsed since the last update
  void update( sf::Time dt );

  //! @brief Push a new scene onto the stack, calling on_exit() on the scene below it.
  //! @param scene The scene to push
  //! @param mode Which components (if any) to copy from the outgoing scene's registry into the new scene's registry
  void push( std::unique_ptr<IScene> scene, RegCopyMode mode = RegCopyMode::NONE );
  //! @brief Push a new overlay scene - do not call on_exit() for the scene below this on the stack
  //! @param scene The scene to push
  //! @param mode Which components (if any) to copy from the outgoing scene's registry into the new scene's registry
  void push_no_exit( std::unique_ptr<IScene> scene, RegCopyMode mode = RegCopyMode::NONE );

  //! @brief Pop the current scene
  //! @param mode Which components (if any) to copy from the popped scene's registry into the scene beneath it
  void pop( RegCopyMode mode = RegCopyMode::NONE );
  //! @brief Pop the current overlay scene - do not call on_enter() for the scene below this on the stack
  //! @param mode Which components (if any) to copy from the popped scene's registry into the scene beneath it
  void pop_no_exit( RegCopyMode mode = RegCopyMode::NONE );

  //! @brief Replace the current scene with a new one
  //! @param scene The scene to replace the current scene with
  //! @param mode Which components (if any) to copy from the outgoing scene's registry into the new scene's registry
  void replace( std::unique_ptr<IScene> scene, RegCopyMode mode = RegCopyMode::NONE );
  //! @brief Replace the current scene with a new one - do not call on_exit() for the replaced scene
  //! @param scene The scene to replace the current scene with
  //! @param mode Which components (if any) to copy from the outgoing scene's registry into the new scene's registry
  void replace_no_exit( std::unique_ptr<IScene> scene, RegCopyMode mode = RegCopyMode::NONE );

  //! @brief Get a pointer to the current active scene
  //! @return IScene*
  IScene *current();

  //! @brief Event handler for scene manager events
  //! @param event The scene manager event describing which transition was requested
  void handle_events( const Events::SceneManagerEvent &event );

private:
  //! @brief Helper function to inject the current scene's registry into the system store
  void inject_current_scene_registry_into_systems();

  //! @brief Runs `callable` asynchronously while displaying an animated "Loading..." screen and
  //!        pumping window events, so the window doesn't appear to hang during long scene setup.
  //! @tparam Callable Callable type invoked with no arguments; its return value is discarded
  //! @param callable The work to perform asynchronously (e.g. a scene's on_init()/on_enter()/on_exit())
  //! @param loading_texture Unused; reserved for a future loading screen background image
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
      while ( const std::optional event = m_window.pollEvent() )
      {
        if ( event->is<sf::Event::Closed>() )
        {
          m_window.close();
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

      m_window.clear( sf::Color::Black );
      m_window.draw( loading_text );
      m_window.display();
    }

    // Ensure any exceptions from the worker thread are propagated
    future.get();
  }

  //! @brief Non-owning reference to the OpenGL window
  sf::RenderWindow &m_window;

  //! @brief Non-owning reference to the sound bank
  Audio::SoundBank &m_sound_bank;

  //! @brief Non-owning reference to the system store
  Sys::Store &m_system_store;

  //! @brief Scene stack managing active scenes
  SceneStack m_scene_stack;

  //! @brief Splash screen texture
  sf::Texture m_splash_texture{ "res/textures/splash.png" };

  //! @brief Non-owning reference to the navigation event dispatcher
  entt::dispatcher &m_nav_event_dispatcher;

  //! @brief Non-owning reference to the scene manager event dispatcher
  entt::dispatcher &m_scenemanager_event_dispatcher;

  //! @brief Used to transfer components from outgoing scene registry to an incoming scene registry
  RegistryTransfer m_reg_xfer;
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_SCENEMANAGER_HPP__
