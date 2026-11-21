#include <Audio/SoundBank.hpp>
#include <Components/Player/Cadaver.hpp>
#include <Components/Player/Wealth.hpp>
#include <SceneControl/Events/ProcessLevelCompleteSceneInputEvent.hpp>
#include <SceneControl/Scenes/LevelCompleteScene.hpp>
#include <Systems/FootstepSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/PlayerSystem.hpp>
#include <Systems/Render/RenderMenuSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Utils/Player.hpp>

#include <SFML/System/Time.hpp>

namespace Game::Scene
{

void LevelCompleteScene::on_init() { /* init entities */ }
void LevelCompleteScene::on_enter()
{
  SPDLOG_DEBUG( "Entering {}", get_name() );
  auto &persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  persistent_sys.initialize_component_registry();
  persistent_sys.load_state();
  m_sound_bank.stop_all();
}

void LevelCompleteScene::on_exit()
{
  SPDLOG_DEBUG( "Exiting {}", get_name() );
  m_sys.find<Sys::Store::Type::FootstepSystem>().stop_footsteps_sound();
  m_reg.clear();
}

void LevelCompleteScene::do_update( sf::Time dt )
{
  m_sys.find<Sys::Store::Type::FootstepSystem>().stop_footsteps_sound();

  auto &wealth = Utils::Player::get_wealth( m_reg );
  auto &cadaver_cmp = Utils::Player::get_cadaver( m_reg );

  static constexpr float kPlantCheckIntervalHz = 1.0f;
  m_scorecheck_accumulator += dt;
  if ( m_scorecheck_accumulator.asSeconds() >= 1.f / kPlantCheckIntervalHz )
  {
    // convert one collected cadaver per tick into wealth
    for ( auto &[key, collected] : cadaver_cmp )
    {
      if ( not collected ) continue;
      collected = false;
      wealth.wealth += 10;
      m_cadaver_wealth += 10;
      m_sound_bank.get_effect( "get_loot" ).play();
      break;
    }
    m_scorecheck_accumulator = sf::Time::Zero;
  }

  const bool allow_continue = cadaver_cmp.count() == 0;
  auto &render_menu_sys = m_sys.find<Sys::Store::Type::RenderMenuSystem>();
  render_menu_sys.render_victory_screen( allow_continue, m_cadaver_wealth );
}

entt::registry &LevelCompleteScene::registry() { return m_reg; }

} // namespace Game::Scene