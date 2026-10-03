#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/RectBounds.hpp>
#include <Shaders/DarkModeShader.hpp>
#include <Shaders/DrippingBloodShader.hpp>
#include <Shaders/FloodWaterShader.hpp>
#include <Shaders/MistShader.hpp>
#include <Shaders/NightStaticShader.hpp>
#include <Shaders/TitleScreenShader.hpp>
#include <Sprites/VertexSprite.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Optimizations.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <entt/entity/fwd.hpp>
#include <imgui-SFML.h>

namespace Game::Sys
{

RenderSystem::RenderSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  SPDLOG_DEBUG( "RenderSystem constructor called" );
}

RenderSystem::~RenderSystem() = default;

void RenderSystem::draw_screen( const sf::Drawable &drawable )
{
  auto &target = active_render_target();
  const sf::View previous_view = target.getView();
  target.setView( m_window.getDefaultView() );
  target.draw( drawable );
  target.setView( previous_view );
}

void RenderSystem::draw_world( const sf::Drawable &drawable )
{
  auto &target = active_render_target();
  const sf::View previous_view = target.getView();
  target.setView( s_world_view );
  target.draw( drawable );
  target.setView( previous_view );
}

void RenderSystem::render_text( std::string text, unsigned int size, sf::Vector2f position, Alignment align, float letter_spacing,
                                sf::Color fill_color, sf::Color outline_color )
{
  sf::Text title_text( m_font, text, size );
  title_text.setFillColor( fill_color );
  title_text.setOutlineColor( outline_color );
  if ( outline_color != sf::Color::Transparent ) { title_text.setOutlineThickness( 5.f ); }
  title_text.setLetterSpacing( letter_spacing );

  // if requested center aligned then we ignore the user provided x position
  sf::Vector2f final_position = position;
  if ( align == Alignment::CENTER )
  {
    sf::FloatRect text_bounds = title_text.getLocalBounds();
    sf::Vector2u display_size = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );
    final_position.x = display_size.x * 0.5f;
    final_position.x -= text_bounds.size.x * 0.5f;
  }

  title_text.setPosition( final_position );

  // Create a black background rectangle for the title text
  float cell_padding = 20;
  sf::RectangleShape title_bg;
  sf::FloatRect title_bounds = title_text.getLocalBounds();
  title_bg.setSize( { title_bounds.size.x + ( cell_padding * 2.f ), title_bounds.size.y + ( cell_padding * 2.f ) } );
  title_bg.setFillColor( sf::Color::Black );
  title_bg.setPosition( { final_position.x + title_bounds.position.x - cell_padding, final_position.y + title_bounds.position.y - cell_padding } );

  active_render_target().draw( title_bg );
  active_render_target().draw( title_text );
}

void RenderSystem::safe_render_sprite_to_target( sf::RenderTarget &target, const Sys::SpriteKey &sprite_type, const sf::FloatRect &pos_cmp,
                                                 std::size_t sprite_index, sf::Vector2f scale, uint8_t alpha, sf::Vector2f origin, sf::Angle angle )
{

  try
  {
    const auto &sprite = Sys::SpriteStore::instance().get( sprite_type );

    if ( sprite_index < sprite.sheet_size() )
    {
      const auto &readonly_texture = sprite.texture();
      Sys::VertexSprite sprite_buffer( sprite.get_vertex_array( sprite_index ), readonly_texture );

      // Adjust position to compensate for origin offset when origin != (0,0)
      sf::Vector2f adjusted_position = pos_cmp.position;
      if ( origin != sf::Vector2f( 0.f, 0.f ) ) { adjusted_position += origin; }
      sprite_buffer.setPosition( adjusted_position );

      sprite_buffer.setScale( scale );
      sprite_buffer.set_pick_opacity( alpha );
      sprite_buffer.setOrigin( origin );
      sprite_buffer.setRotation( angle );
      target.draw( sprite_buffer );
    }
    else
    {
      SPDLOG_WARN( "Unable to get idx: {}. {} has sprite count = {}", sprite_index, sprite_type, sprite.sheet_size() );
      render_fallback_square_to_target( target, pos_cmp, sf::Color::Cyan );
    }
  } catch ( const std::out_of_range &e )
  {
    // SPDLOG_WARN( "Missing sprite '{}' in map, rendering fallback square", sprite_type );
    render_fallback_square_to_target( target, pos_cmp, sf::Color::Magenta );
  }
}

void RenderSystem::render_fallback_square_to_target( sf::RenderTarget &target, const sf::FloatRect &pos_cmp, const sf::Color &color )
{
  sf::RectangleShape fallback_square( Constants::kGridSizePxF );
  fallback_square.setPosition( pos_cmp.position );
  fallback_square.setFillColor( color );
  fallback_square.setOutlineColor( sf::Color::White );
  fallback_square.setOutlineThickness( 1.f );
  target.draw( fallback_square ); // Draw to specified target
}

void RenderSystem::safe_render_sprite_screen( const Sys::SpriteKey &sprite_type, const sf::FloatRect &pos_cmp, std::size_t sprite_index,
                                              sf::Vector2f scale, uint8_t alpha, sf::Vector2f origin, sf::Angle angle )
{
  auto &target = active_render_target();
  const sf::View previous_view = target.getView();
  target.setView( get_screen_view() );
  safe_render_sprite_to_target( target, sprite_type, pos_cmp, sprite_index, scale, alpha, origin, angle );
  target.setView( previous_view );
}

// Keep the original for backwards compatibility
void RenderSystem::safe_render_sprite_world( const Sys::SpriteKey &sprite_type, const sf::FloatRect &pos_cmp, std::size_t sprite_index,
                                             sf::Vector2f scale, uint8_t alpha, sf::Vector2f origin, sf::Angle angle )
{
  auto &target = active_render_target();
  const sf::View previous_view = target.getView();
  target.setView( get_world_view() );
  safe_render_sprite_to_target( target, sprite_type, pos_cmp, sprite_index, scale, alpha, origin, angle );
  target.setView( previous_view );
}

void RenderSystem::render_fallback_square_world( const sf::FloatRect &pos_cmp, const sf::Color &color )
{
  auto &target = active_render_target();
  const sf::View previous_view = target.getView();
  target.setView( get_world_view() );
  render_fallback_square_to_target( target, pos_cmp, color );
  target.setView( previous_view );
}

void RenderSystem::render_rectbounds( Cmp::RectBounds &bounds, sf::Color color )
{
  if ( not Utils::is_visible_in_view( RenderSystem::get_world_view(), bounds.getBounds() ) ) return;
  sf::RectangleShape rect;
  rect.setSize( bounds.size() );
  rect.setPosition( bounds.position() );
  rect.setFillColor( sf::Color::Transparent );
  rect.setOutlineColor( color );
  rect.setOutlineThickness( 1.f );
  draw_world( rect );
}

sf::View RenderSystem::s_world_view{};
ZOrderQueue RenderSystem::s_zorder_queue{};

} // namespace Game::Sys
