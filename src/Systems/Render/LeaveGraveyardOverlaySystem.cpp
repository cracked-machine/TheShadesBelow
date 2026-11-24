#include <Audio/SoundBank.hpp>
#include <Systems/CartHorseSystem.hpp>
#include <Systems/Render/LeaveGraveyardOverlaySystem.hpp>
#include <Systems/Stores/SystemStore.hpp>

namespace Game::Sys
{

LeaveGraveyardOverlaySystem::LeaveGraveyardOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, Store &sys )
    : BaseOverlaySystem( reg, window, sound_bank ),
      m_sys( sys )
{
  std::ignore = get_systems_event_queue().sink<Events::OverlayEvent>().connect<&LeaveGraveyardOverlaySystem::on_overlay_event>( this );
}

Cmp::SceneSettings::Paused *LeaveGraveyardOverlaySystem::find_paused()
{
  auto paused_view = reg().view<Cmp::SceneSettings::Paused>();
  if ( paused_view.empty() ) { return nullptr; }
  return &paused_view.get<Cmp::SceneSettings::Paused>( paused_view.front() );
}

bool LeaveGraveyardOverlaySystem::is_open()
{
  const auto *paused = find_paused();
  return paused and paused->reason == Cmp::SceneSettings::Paused::Reason::LEAVE_PROMPT;
}

void LeaveGraveyardOverlaySystem::open()
{
  auto *paused = find_paused();
  if ( not paused or paused->reason != Cmp::SceneSettings::Paused::Reason::NONE ) { return; }
  paused->reason = Cmp::SceneSettings::Paused::Reason::LEAVE_PROMPT;
  m_selected = kNoRow;
  // like a tooltip pause: one-shot effects (the horse's neigh) play out, but looping ones must not play on
  cancel_bow_draw();
  m_sound_bank.suspend_effect_loops();
  SPDLOG_DEBUG( "Leave graveyard prompt opened" );
}

void LeaveGraveyardOverlaySystem::close()
{
  if ( not is_open() ) { return; }
  find_paused()->reason = Cmp::SceneSettings::Paused::Reason::NONE;
  m_sound_bank.resume_effect_loops();
  SPDLOG_DEBUG( "Leave graveyard prompt closed" );
}

void LeaveGraveyardOverlaySystem::update( [[maybe_unused]] sf::Time dt )
{
  if ( not is_open() ) { return; }
  render_selection_menu( "Do you want to leave the grave yard?", "", true, kRows, m_selected, "Up/Down to choose, Enter to confirm (Y / N)" );
}

void LeaveGraveyardOverlaySystem::on_overlay_event( const Events::OverlayEvent &ev )
{
  using Action = Events::OverlayEvent::OverlayActions;
  if ( not is_open() ) { return; }

  bool leave = false;
  switch ( ev.action )
  {
    case Action::LEAVE_PREV:
    case Action::LEAVE_NEXT:
      m_selected = step_selection( m_selected, kRows.size(), ev.action == Action::LEAVE_NEXT );
      return;
    case Action::LEAVE_CONFIRM:
      leave = m_selected == kYesRow;
      break;
    case Action::LEAVE_YES:
      leave = true;
      break;
    case Action::LEAVE_NO:
      break;
    default:
      return;
  }

  close();
  if ( leave ) { m_sys.find<Store::Type::CartHorseSystem>().leave_level(); }
}

} // namespace Game::Sys
