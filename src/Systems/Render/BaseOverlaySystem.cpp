#include <Audio/SoundBank.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Systems/Render/BaseOverlaySystem.hpp>

#include <SFML/Graphics/RectangleShape.hpp>

#include <algorithm>
#include <cmath>

namespace Game::Sys
{

BaseOverlaySystem::BaseOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : RenderSystem( reg, window, sound_bank )
{
}

void BaseOverlaySystem::render_outlines( const Render::UiData &ui_data )
{
  sf::RectangleShape rect;
  for ( const auto &outline : ui_data.m_outlines )
  {
    rect.setSize( outline.rect.size );
    rect.setPosition( outline.rect.position );
    rect.setFillColor( outline.fill_color );
    rect.setOutlineColor( outline.line_color );
    rect.setOutlineThickness( static_cast<float>( outline.line_thickness ) );
    rect.setRotation( outline.rotation );
    draw_screen( rect );
  }
}

unsigned int BaseOverlaySystem::scaled_char_size( unsigned int design_size, const Render::UiData *ui_data )
{
  return std::max( 1u, static_cast<unsigned int>( std::lround( static_cast<float>( design_size ) * ui_scale( ui_data ) ) ) );
}

void BaseOverlaySystem::on_modal_open()
{
  // the mouse release is swallowed while a modal overlay is open, so abandon any draw now rather than leave it pending.
  // Cancel before pausing the effects so the draw sfx is stopped, not paused and later resumed.
  get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::CANCEL_BOW ) );
  m_sound_bank.pause_effects();
}

void BaseOverlaySystem::on_modal_close() { m_sound_bank.resume_effects(); }

} // namespace Game::Sys
