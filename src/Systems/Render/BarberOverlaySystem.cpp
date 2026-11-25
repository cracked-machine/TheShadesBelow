#include <Components/SceneSettings/CurrentScene.hpp>
#include <Events/OverlayEvent.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <Systems/HealingSpringSystem.hpp>
#include <Systems/Render/OverlayArbitrationSystem.hpp>
#include <Systems/Render/BarberOverlaySystem.hpp>
#include <Systems/ShopSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Utils/Utils.hpp>

#include <format>

namespace Game::Sys
{

BarberOverlaySystem::BarberOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, Store &sys )
    : BaseOverlaySystem( reg, window, sound_bank ),
      m_sys( sys )
{
  std::ignore = get_systems_event_queue().sink<Events::OverlayEvent>().connect<&BarberOverlaySystem::on_overlay_event>( this );
}

bool BarberOverlaySystem::is_active()
{
  if ( not Utils::is_current_scene( reg(), Cmp::SceneSettings::SceneId::BARBER_SURGEON ) ) return false;
  if ( OverlayArbitrationSystem::active_modal( reg() ) != OverlayArbitrationSystem::Modal::NONE ) return false;
  if ( m_sys.find<Store::Type::HealingSpringSystem>().is_exit_pending() ) return false;
  return ShopSystem::is_heal_menu_open( reg() );
}

void BarberOverlaySystem::update( [[maybe_unused]] sf::Time dt )
{
  if ( not is_active() )
  {
    // start from the top row on the next visit to the surgeon
    if ( not ShopSystem::is_heal_menu_open( reg() ) ) { m_selected = 0; }
    return;
  }
  const bool affordable = m_sys.find<Store::Type::ShopSystem>().can_afford_heal();
  render_selection_menu( "Barber Surgeon", std::format( "Cost: {} gold", ShopSystem::kHealGoldCost ), affordable,
                         ShopSystem::kHealStatLabels, m_selected, "Scroll to choose, E to buy" );
}

void BarberOverlaySystem::on_overlay_event( const Events::OverlayEvent &ev )
{
  using Action = Events::OverlayEvent::OverlayActions;
  if ( ev.action != Action::HEAL_PREV and ev.action != Action::HEAL_NEXT and ev.action != Action::HEAL_PURCHASE ) return;
  if ( not is_active() ) return;

  if ( ev.action != Action::HEAL_PURCHASE )
  {
    m_selected = step_selection( m_selected, ShopSystem::kHealStatLabels.size(), ev.action == Action::HEAL_NEXT );
    return;
  }
  const auto stat = static_cast<ShopSystem::HealStat>( m_selected );
  if ( not m_sys.find<Store::Type::ShopSystem>().buy_heal( stat ) ) return;
  m_sys.find<Store::Type::HealingSpringSystem>().request_delayed_exit( Events::SceneManagerEvent::Type::EXIT_BARBERSURGEON );
}

} // namespace Game::Sys
