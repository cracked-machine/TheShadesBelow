#include <Components/SceneSettings/CurrentScene.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <Systems/HealingSpringSystem.hpp>
#include <Systems/Render/OverlayArbitrationSystem.hpp>
#include <Systems/Render/SpringOverlaySystem.hpp>
#include <Systems/ShopSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Utils/Utils.hpp>

#include <format>

namespace Game::Sys
{

namespace
{
//! @brief The healing spring is paid for with anima
constexpr auto kCurrency = ShopSystem::HealCurrency::ANIMA;
} // namespace

SpringOverlaySystem::SpringOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, Store &sys )
    : BaseOverlaySystem( reg, window, sound_bank ),
      m_sys( sys )
{
  std::ignore = get_systems_event_queue().sink<Events::OverlayEvent>().connect<&SpringOverlaySystem::on_overlay_event>( this );
}

bool SpringOverlaySystem::is_active()
{
  if ( not Utils::is_current_scene( reg(), Cmp::SceneSettings::SceneId::HEALING_SPRING ) ) return false;
  if ( OverlayArbitrationSystem::active_modal( reg() ) != OverlayArbitrationSystem::Modal::NONE ) return false;
  if ( m_sys.find<Store::Type::HealingSpringSystem>().is_exit_pending() ) return false;
  return ShopSystem::is_heal_menu_open( reg() );
}

void SpringOverlaySystem::update( [[maybe_unused]] sf::Time dt )
{
  if ( not is_active() )
  {
    // start from the top row on the next visit to the spring
    if ( not ShopSystem::is_heal_menu_open( reg() ) ) { m_selected = 0; }
    return;
  }
  const bool affordable = m_sys.find<Store::Type::ShopSystem>().can_afford_heal( kCurrency );
  render_selection_menu( "Healing Spring", std::format( "Cost: {} anima", ShopSystem::heal_cost( kCurrency ) ), affordable,
                         ShopSystem::kHealStatLabels, m_selected, "Scroll to choose, E to buy" );
}

void SpringOverlaySystem::on_overlay_event( const Events::OverlayEvent &ev )
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
  if ( not m_sys.find<Store::Type::ShopSystem>().buy_heal( stat, kCurrency ) ) return;
  m_sys.find<Store::Type::HealingSpringSystem>().request_delayed_exit( Events::SceneManagerEvent::Type::EXIT_SACREDSPRING );
}

} // namespace Game::Sys
