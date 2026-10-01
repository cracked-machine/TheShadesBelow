#include <Components/Inventory/Grimoire.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Systems/GrimoireSystem.hpp>

#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>

namespace Game::Sys
{

GrimoireSystem::GrimoireSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&GrimoireSystem::on_player_action>( this );
}

void GrimoireSystem::update( sf::Time dt )
{
  overlay_debounce += dt;
  if ( m_grimoire_overlay_open ) { enable_grimoire_overlay(); }
  else { disable_grimoire_overlay(); }
}

void GrimoireSystem::on_player_action( Events::PlayerActionEvent ev )
{
  if ( ev.action != Events::PlayerActionEvent::GameActions::TOGGLE_GRIMOIRE ) return;
  static float overlay_debounce_threshold = 1.f;
  if ( overlay_debounce.asSeconds() >= overlay_debounce_threshold )
  {
    m_grimoire_overlay_open = not m_grimoire_overlay_open;
    overlay_debounce = sf::Time::Zero;
  }
}

void GrimoireSystem::enable_grimoire_overlay()
{
  for ( auto [grimoire_entt, grimoire_cmp] : reg().view<Cmp::Grimoire>().each() )
  {
    grimoire_cmp.is_enabled = true;
  }
}

void GrimoireSystem::disable_grimoire_overlay()
{
  for ( auto [grimoire_entt, grimoire_cmp] : reg().view<Cmp::Grimoire>().each() )
  {
    grimoire_cmp.is_enabled = false;
  }
}

} // namespace Game::Sys