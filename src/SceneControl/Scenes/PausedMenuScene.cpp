#include <Audio/SoundBank.hpp>
#include <Components/Persistent/EffectsVolume.hpp>
#include <Components/Persistent/MusicVolume.hpp>
#include <Events/PauseClocksEvent.hpp>
#include <Events/ResumeClocksEvent.hpp>
#include <SceneControl/Events/ProcessPausedMenuSceneInputEvent.hpp>
#include <SceneControl/Scenes/PausedMenuScene.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderMenuSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>

namespace Game::Scene
{

void PausedMenuScene::on_init() { SPDLOG_DEBUG( "Initializing PausedMenuScene" ); }

void PausedMenuScene::on_enter()
{
  SPDLOG_DEBUG( "Entering  {}", get_name() );
  auto &persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  persistent_sys.initialize_component_registry();
  persistent_sys.load_state();

  Sys::PersistSystem::get_systems_event_queue().trigger( Events::PauseClocksEvent() );
}

void PausedMenuScene::on_exit()
{
  SPDLOG_DEBUG( "Exiting {}", get_name() );
  auto &persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();

  // save the state because there are UI controls for volume.
  persistent_sys.save_state();

  Sys::PersistSystem::get_systems_event_queue().trigger( Events::ResumeClocksEvent() );
}

void PausedMenuScene::do_update( [[maybe_unused]] sf::Time dt )
{

  auto &render_menu_sys = m_sys.find<Sys::Store::Type::RenderMenuSystem>();

  // check for keyboard/window events to keep window responsive
  render_menu_sys.render_paused( dt );
  std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );

  // save persistent settings
  auto &persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  persistent_sys.save_state();

  // update music/sfx volumes with persistent settings
  auto &effects_volume = Sys::PersistSystem::get<Cmp::Persist::EffectsVolume>( m_reg ).get_value();
  m_sound_bank.update_effects_volume( effects_volume );
  auto &music_volume = Sys::PersistSystem::get<Cmp::Persist::MusicVolume>( m_reg ).get_value();
  m_sound_bank.update_music_volume( music_volume );
}

entt::registry &PausedMenuScene::registry() { return m_reg; }

} // namespace Game::Scene