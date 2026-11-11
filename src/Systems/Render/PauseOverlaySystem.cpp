#include <Audio/SoundBank.hpp>
#include <Components/Inventory/Grimoire.hpp>
#include <Components/Persistent/EffectsVolume.hpp>
#include <Components/Persistent/MusicVolume.hpp>
#include <Components/SceneSettings/Paused.hpp>
#include <Components/SceneSettings/ShowDebugStats.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Events/SaveSettingsEvent.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/PauseOverlaySystem.hpp>

#include <SFML/Graphics/Text.hpp>

#include <algorithm>
#include <imgui-SFML.h>
#include <imgui.h>

namespace Game::Sys
{

namespace
{
constexpr unsigned int kTitleFontSize = 96;
constexpr unsigned int kHintFontSize = 48;
constexpr float kTitleY = 100.f;
constexpr float kHintY = 220.f;
constexpr float kSlidersY = 320.f;
constexpr float kSlidersWidth = 600.f;
constexpr float kTextOutlineThickness = 4.f;
} // namespace

PauseOverlaySystem::PauseOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : RenderSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&PauseOverlaySystem::on_player_action>( this );
}

bool PauseOverlaySystem::is_paused() const
{
  auto paused_view = reg().view<const Cmp::SceneSettings::Paused>();
  if ( paused_view.empty() ) { return false; }
  return paused_view.get<const Cmp::SceneSettings::Paused>( paused_view.front() ).enabled;
}

void PauseOverlaySystem::on_player_action( Events::PlayerActionEvent ev )
{
  if ( ev.action != Events::PlayerActionEvent::GameActions::PAUSE_TOGGLE ) { return; }

  auto paused_view = reg().view<Cmp::SceneSettings::Paused>();
  if ( paused_view.empty() ) { return; }
  auto &paused = paused_view.get<Cmp::SceneSettings::Paused>( paused_view.front() );

  if ( not paused.enabled )
  {
    // the grimoire and the pause overlay each pump the one ImGui frame allowed per frame, so they are never shown together
    auto grimoire_view = reg().view<const Cmp::Grimoire>();
    if ( not grimoire_view.empty() and grimoire_view.get<const Cmp::Grimoire>( grimoire_view.front() ).is_enabled ) { return; }

    paused.enabled = true;
    // the mouse release is swallowed while paused, so abandon any draw now rather than leave it pending.
    // Cancel before pausing the effects so the draw sfx is stopped, not paused and later resumed.
    get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::CANCEL_BOW ) );
    m_sound_bank.pause_effects();
    SPDLOG_DEBUG( "Game paused" );
  }
  else
  {
    paused.enabled = false;
    // the volume sliders may have changed the persistent settings
    get_systems_event_queue().trigger( Events::SaveSettingsEvent() );
    m_sound_bank.resume_effects();
    SPDLOG_DEBUG( "Game resumed" );
  }
}

void PauseOverlaySystem::force_resume()
{
  auto paused_view = reg().view<Cmp::SceneSettings::Paused>();
  if ( paused_view.empty() ) { return; }
  auto &paused = paused_view.get<Cmp::SceneSettings::Paused>( paused_view.front() );
  if ( not paused.enabled ) { return; }
  paused.enabled = false;
  m_sound_bank.resume_effects();
}

void PauseOverlaySystem::render( sf::Time dt )
{
  if ( not is_paused() )
  {
    // the events forwarded to ImGui on the frame the overlay closed are still queued: pump one empty frame to consume
    // them, so they aren't replayed onto the menu widgets the next time a menu opens. The F3 debug panels pump this
    // frame themselves when shown, so don't pump a second one.
    auto debug_view = reg().view<const Cmp::SceneSettings::ShowDebugStats>();
    const bool debug_shown = not debug_view.empty() and debug_view.get<const Cmp::SceneSettings::ShowDebugStats>( debug_view.front() ).enabled;
    if ( m_imgui_active and not debug_shown )
    {
      ImGui::SFML::Update( m_window, std::max( dt, sf::microseconds( 1 ) ) );
      ImGui::SFML::Render( m_window );
    }
    m_imgui_active = false;
    return;
  }
  m_imgui_active = true;

  const sf::Vector2f display_size( m_window.getSize() );

  // screen-space text, outlined so it stays readable over the frozen world
  auto draw_centered_text = [&]( const char *str, unsigned int size, float y )
  {
    sf::Text text( m_font, str, size );
    text.setFillColor( sf::Color::White );
    text.setOutlineColor( sf::Color::Black );
    text.setOutlineThickness( kTextOutlineThickness );
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setPosition( { ( display_size.x - bounds.size.x ) * 0.5f - bounds.position.x, y } );
    draw_screen( text );
  };
  draw_centered_text( "Paused", kTitleFontSize, kTitleY );
  draw_centered_text( "Press P to continue", kHintFontSize, kHintY );

  // ImGui asserts on a non-positive DeltaTime, so never hand it a zero frame time
  ImGui::SFML::Update( m_window, std::max( dt, sf::microseconds( 1 ) ) );

  ImGui::SetNextWindowPos( ImVec2( ( display_size.x - kSlidersWidth ) * 0.5f, kSlidersY ) );
  ImGui::SetNextWindowSize( ImVec2( kSlidersWidth, 0.f ) );
  constexpr ImGuiWindowFlags kWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;
  if ( ImGui::Begin( "##pause_overlay", nullptr, kWindowFlags ) )
  {
    auto &music_volume = Sys::PersistSystem::get<Cmp::Persist::MusicVolume>( reg() );
    ImGui::SliderFloat( "Music Volume", &music_volume.get_value(), 0.f, 100.f, "%.1f" );

    auto &effects_volume = Sys::PersistSystem::get<Cmp::Persist::EffectsVolume>( reg() );
    ImGui::SliderFloat( "Effects Volume", &effects_volume.get_value(), 0.f, 100.f, "%.1f" );

    m_sound_bank.update_music_volume( music_volume.get_value() );
    m_sound_bank.update_effects_volume( effects_volume.get_value() );
  }
  ImGui::End();

  ImGui::SFML::Render( m_window );
}

} // namespace Game::Sys
