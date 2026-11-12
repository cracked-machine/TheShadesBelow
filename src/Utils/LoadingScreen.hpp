#ifndef SRC_UTILS_LOADINGSCREEN_HPP__
#define SRC_UTILS_LOADINGSCREEN_HPP__

#include <Utils/Constants.hpp>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/View.hpp>

#include <algorithm>
#include <cmath>

namespace Game::Utils
{

//! @brief Uniform scale that fits the loading screen's design resolution (Constants::kFallbackDisplaySize, the size the
//! splash texture and caption were laid out at) inside `window_size` without distortion.
//! @param window_size the window size in pixels
//! @return min( window.x / design.x, window.y / design.y ), or 1 for a zero-sized window
inline float loading_screen_scale( sf::Vector2u window_size )
{
  const sf::Vector2f design( Constants::kFallbackDisplaySize );
  if ( window_size.x == 0u or window_size.y == 0u ) return 1.f;
  return std::min( static_cast<float>( window_size.x ) / design.x, static_cast<float>( window_size.y ) / design.y );
}

//! @brief Size and place the "Loading..." caption for the window's current resolution: its character size, outline and
//! offset from the window centre are scaled from their design-space (1920x1080) values.
//! @param window the window the caption is drawn to
//! @param loading_text the caption to update
inline void layout_loading_text( const sf::RenderWindow &window, sf::Text &loading_text )
{
  constexpr unsigned int kDesignCharSize = 48;
  constexpr sf::Vector2f kDesignOffsetFromCentre{ -50.f, 100.f };
  constexpr float kDesignOutlineThickness = 2.f;

  const sf::Vector2f window_size( window.getSize() );
  const float scale = loading_screen_scale( window.getSize() );
  loading_text.setCharacterSize( std::max( 1u, static_cast<unsigned int>( std::lround( static_cast<float>( kDesignCharSize ) * scale ) ) ) );
  loading_text.setOutlineThickness( kDesignOutlineThickness * scale );
  loading_text.setPosition( ( window_size / 2.f ) + ( kDesignOffsetFromCentre * scale ) );
}

//! @brief Draw one loading screen frame: the splash texture scaled to fit the window (uniformly, centred, letterboxed in
//! black) with the caption over it. Drawn in window pixel coordinates; the window's current view is restored afterwards.
//! @param window the window to draw to (clear/display are left to the caller)
//! @param splash_texture the splash background
//! @param loading_text the caption, already laid out by layout_loading_text()
inline void draw_loading_frame( sf::RenderWindow &window, const sf::Texture &splash_texture, const sf::Text &loading_text )
{
  const sf::View previous_view = window.getView();
  const sf::Vector2f window_size( window.getSize() );
  window.setView( sf::View( sf::FloatRect( { 0.f, 0.f }, window_size ) ) );

  const sf::Vector2f texture_size( splash_texture.getSize() );
  if ( texture_size.x > 0.f and texture_size.y > 0.f )
  {
    // fit the splash itself, which need not be authored at the design resolution
    const float scale = std::min( window_size.x / texture_size.x, window_size.y / texture_size.y );
    sf::Sprite splash( splash_texture );
    splash.setScale( { scale, scale } );
    splash.setPosition( ( window_size - ( texture_size * scale ) ) / 2.f );
    window.draw( splash );
  }
  window.draw( loading_text );

  window.setView( previous_view );
}

} // namespace Game::Utils

#endif // SRC_UTILS_LOADINGSCREEN_HPP__
