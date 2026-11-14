#include <Audio/SoundBank.hpp>
#include <Components/Persistent/EffectsVolume.hpp>
#include <Components/Persistent/MusicVolume.hpp>
#include <Components/SceneSettings/Paused.hpp>
#include <Events/SaveSettingsEvent.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/PauseOverlaySystem.hpp>

#include <SFML/Graphics/Text.hpp>

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
    : BaseOverlaySystem( reg, window, sound_bank )
{
}

Cmp::SceneSettings::Paused *PauseOverlaySystem::find_paused()
{
  auto paused_view = reg().view<Cmp::SceneSettings::Paused>();
  if ( paused_view.empty() ) { return nullptr; }
  return &paused_view.get<Cmp::SceneSettings::Paused>( paused_view.front() );
}

bool PauseOverlaySystem::is_open()
{
  const auto *paused = find_paused();
  return paused and paused->enabled;
}

void PauseOverlaySystem::open()
{
  auto *paused = find_paused();
  if ( not paused or paused->enabled ) { return; }
  paused->enabled = true;
  on_modal_open();
  SPDLOG_DEBUG( "Game paused" );
}

void PauseOverlaySystem::close()
{
  if ( not is_open() ) { return; }
  // the volume sliders may have changed the persistent settings
  get_systems_event_queue().trigger( Events::SaveSettingsEvent() );
  force_resume();
  SPDLOG_DEBUG( "Game resumed" );
}

void PauseOverlaySystem::force_resume()
{
  auto *paused = find_paused();
  if ( not paused or not paused->enabled ) { return; }
  paused->enabled = false;
  on_modal_close();
}

void PauseOverlaySystem::update( [[maybe_unused]] sf::Time dt )
{
  if ( not is_open() ) { return; }

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
}

} // namespace Game::Sys
