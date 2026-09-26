#include <Components/Persistent/EffectsVolume.hpp>
#include <Factory/ShaderFactory.hpp>
#include <SceneControl/Scenes/TitleScene.hpp>

#include <Audio/SoundBank.hpp>
#include <Components/Persistent/MusicVolume.hpp>
#include <SceneControl/Events/ProcessTitleSceneInputEvent.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderMenuSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Systems/ShaderSystem.hpp>

namespace Game::Scene
{

void TitleScene::on_init()
{

  SPDLOG_DEBUG( "Initializing TitleScene" );

  auto &m_persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();

  // We only have access to DisplayResolution once the PersistSystem is initialized.
  Factory::Shader::add_title( m_sys.find<Sys::Store::Type::ShaderSystem>(), Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( m_reg ) );
}

void TitleScene::on_enter()
{
  SPDLOG_DEBUG( "Entering {}", get_name() );

  auto &m_persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();

  // update fx volumes with persistent settings
  auto &effects_volume = Sys::PersistSystem::get<Cmp::Persist::EffectsVolume>( m_reg ).get_value();
  m_sound_bank.update_effects_volume( effects_volume );
  auto &music_volume = Sys::PersistSystem::get<Cmp::Persist::MusicVolume>( m_reg ).get_value();
  m_sound_bank.get_music( "graveyard_music" ).stop();
  m_sound_bank.update_music_volume( music_volume );

  if ( m_sound_bank.get_music( "title_music" ).getStatus() != sf::Music::Status::Playing )
  {
    m_sound_bank.get_music( "title_music" ).play();
    m_sound_bank.get_music( "title_music" ).setLooping( true );
  }
}

void TitleScene::on_exit()
{
  // Cleanup if needed
  SPDLOG_DEBUG( "Exiting {}", get_name() );
}

void TitleScene::do_update( sf::Time dt )
{
  m_sys.find<Sys::Store::Type::ShaderSystem>().update( dt );
  auto &render_menu_sys = m_sys.find<Sys::Store::Type::RenderMenuSystem>();
  render_menu_sys.render_title();
}

entt::registry &TitleScene::registry() { return m_reg; }

} // namespace Game::Scene