#include <SceneControl/Events/ProcessSettingsMenuSceneInputEvent.hpp>
#include <SceneControl/Scenes/SettingsMenuScene.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderMenuSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>

namespace Game::Scene
{

void SettingsMenuScene::on_init() { SPDLOG_DEBUG( "Initializing SettingsMenuScene" ); }

void SettingsMenuScene::on_enter()
{
  SPDLOG_DEBUG( "Entering  {}", get_name() );
  auto &m_persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  m_persistent_sys.initialize_component_registry();
  m_persistent_sys.load_state();
}
void SettingsMenuScene::on_exit()
{
  SPDLOG_DEBUG( "Exiting  {}", get_name() );
  auto &m_persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  m_persistent_sys.save_state();
}
void SettingsMenuScene::do_update( [[maybe_unused]] sf::Time dt )
{
  auto &m_render_menu_sys = m_sys.find<Sys::Store::Type::RenderMenuSystem>();
  m_render_menu_sys.render_settings( dt );
}

entt::registry &SettingsMenuScene::registry() { return m_reg; }

} // namespace Game::Scene