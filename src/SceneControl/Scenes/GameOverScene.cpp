#include <SceneControl/Scenes/GameOverScene.hpp>

#include <Audio/SoundBank.hpp>
#include <SceneControl/Events/ProcessGameoverSceneInputEvent.hpp>
#include <Systems/FootstepSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/PlayerSystem.hpp>
#include <Systems/Render/RenderMenuSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>

namespace Game::Scene
{

void GameOverScene::on_init() { SPDLOG_DEBUG( "Initializing GameOverScene" ); }
void GameOverScene::on_enter()
{
  SPDLOG_DEBUG( "Entering {}", get_name() );
  auto &persistent_sys = static_cast<Sys::PersistSystem &>( m_sys.find<Sys::Store::Type::PersistSystem>() );
  persistent_sys.initialize_component_registry();
  persistent_sys.load_state();
  m_sound_bank.get_effect( "bubbling_lava" ).stop();
  m_sound_bank.get_effect( "skeleton_moving" ).stop();
}
void GameOverScene::on_exit()
{
  SPDLOG_DEBUG( "Exiting {}", get_name() );
  m_sys.find<Sys::Store::Type::FootstepSystem>().stop_footsteps_sound();
  m_reg.clear();
}

void GameOverScene::do_update( [[maybe_unused]] sf::Time dt )
{
  m_sys.find<Sys::Store::Type::FootstepSystem>().stop_footsteps_sound();

  auto &render_menu_sys = m_sys.find<Sys::Store::Type::RenderMenuSystem>();
  render_menu_sys.render_defeat_screen();
}

entt::registry &GameOverScene::registry() { return m_reg; }

} // namespace Game::Scene