#ifndef SRC_SYSTEMS_RENDER_RENDERSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_RENDERSYSTEM_HPP__

#include <Components/Font.hpp>
#include <Components/Persistent/DisplayResolution.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Render/ZOrderQueue.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Optimizations.hpp>

#include <SFML/System/Vector2.hpp>
#include <entt/entity/fwd.hpp>
#include <functional>
#include <imgui.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace Game::Cmp
{
class RectBounds;
}

namespace Game::Sys
{

//! @brief Base class for the game's render systems (RenderGameSystem, RenderMenuSystem, RenderOverlaySystem,
//! RenderDebugSystem). Owns the shared world view
//! and font, and provides common rendering primitives such as safe sprite rendering with fallback squares, text rendering, and
//! world/screen coordinate conversion.
class RenderSystem : public BaseSystem
{
public:
  //! @brief Construct a new Render System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  RenderSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief polymorphic destructor for derived classes
  virtual ~RenderSystem();

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief Get the current view of the game world. See RenderSystem::kWorldViewSize.
  //! @return const sf::View&
  static const sf::View &get_world_view() { return s_world_view; }

  //! @brief Get the screen resolution view. See Cmp::Persist::DisplayResolution.
  //! @return const sf::View&
  const sf::View &get_screen_view() { return m_window.getDefaultView(); }

protected:
  //! @brief Text alignment options
  enum class Alignment {
    //! @brief Left align text (respects position.x)
    LEFT,
    //! @brief Center align text (ignores position.x)
    CENTER
  };

  //! @brief Draw a column of text lines, top to bottom, at a fixed origin, advancing by `line_height` after each call.
  //! Backed by a per-`cache_key` pool of persistent sf::Text objects (see m_text_column_cache) so that, across frames, each
  //! line reuses the same sf::Text instead of being reconstructed (and having its outline re-generated) from scratch -
  //! these panels can otherwise update every frame at a real cost to frame time.
  struct TextColumn
  {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members) - TextColumn is a short-lived,
    // per-panel-call local (never stored, copy-assigned, or passed around), so the usual dangling-reference
    // risk this check guards against doesn't apply here.
    //! @brief The render system used to draw each text line.
    RenderSystem &self;

    //! @brief Identifies this column's slot in self.m_text_column_cache. Stable across frames for a given panel (e.g.
    //! "npc_list") so the same sf::Text objects are reused call after call.
    std::string cache_key;

    //! @brief Screen position of the first line in the column.
    sf::Vector2f origin;

    //! @brief Font size, in pixels, of each line.
    unsigned int font_size;

    //! @brief Vertical spacing, in pixels, added after each line is drawn.
    float line_height;

    //! @brief Running vertical offset from `origin`, advanced by `line_height` after each call.
    float y_offset{ 0.f };

    //! @brief Index into this column's cache pool of the next line to draw, advanced after each call.
    std::size_t line_index{ 0 };

    //! @brief Draw one line of text at the current column offset, then advance the offset by `line_height`.
    //! @param str The text to draw.
    //! @param color Fill colour of the text.
    void operator()( const std::string &str, sf::Color color = sf::Color::White )
    {
      auto &pool = self.m_text_column_cache[cache_key];
      if ( line_index >= pool.size() )
      {
        // Outline colour/thickness are the same for every line ever drawn through this struct, so they only need
        // setting once per pooled sf::Text - re-applying them every frame is what forces SFML to regenerate the
        // outline geometry for every visible line, every frame.
        sf::Text text( self.m_font, str, font_size );
        text.setOutlineColor( sf::Color::Black );
        text.setOutlineThickness( 1.f );
        pool.push_back( std::move( text ) );
      }

      sf::Text &text = pool[line_index];
      text.setString( str );
      text.setFillColor( color );
      text.setPosition( { origin.x, origin.y + y_offset } );
      self.draw_screen( text );

      y_offset += line_height;
      ++line_index;
    }
  };

  //! @brief Dimension for `s_world_view`.
  constexpr static sf::Vector2u kWorldViewSize{ 300u, 200u };

  //! @brief `kWorldViewSize` as a float vector, for use in float-based calculations.
  constexpr static sf::Vector2f kWorldViewSizeF{ static_cast<float>( kWorldViewSize.x ), static_cast<float>( kWorldViewSize.y ) };

  //! @brief Convert the world position to the equivalent position in the screen view
  //! @param world_pos
  //! @return sf::Vector2f
  sf::Vector2f world_to_screen( sf::Vector2f world_pos ) const { return sf::Vector2f( m_window.mapCoordsToPixel( world_pos, get_world_view() ) ); }

  //! @brief Draw in screen view coordinates. This restores the view afterwards.
  //! @param drawable
  void draw_screen( const sf::Drawable &drawable );

  //! @brief Draw in world view coordinates. This restores the view afterwards.
  //! @param drawable
  void draw_world( const sf::Drawable &drawable );

  //! @brief getter for `m_current_target`
  //! @return sf::RenderTarget&
  sf::RenderTarget &active_render_target() { return m_current_target; }

  //! @brief Set the render target object to something other than sf::RenderWindow.
  //! @param target
  void set_render_target( sf::RenderTarget &target ) { m_current_target = target; }

  //! @brief Restore render target back to sf::RenderWindow.
  void restore_render_target() { m_current_target = m_window; }

  //! @brief Renders text to the screen with specified formatting and alignment options.
  //!
  //! @param text The string content to be rendered
  //! @param size The font size for the text in pixels
  //! @param position The screen coordinates where the text should be positioned
  //! @param align The alignment mode for the text (left or center). Left will
  //! respect position.x, center will ignore it.
  //! @param letter_spacing Optional letter spacing multiplier for the text (default: 1.f)
  //! @param fill_color Optional color for the text fill (default: White)
  //! @param outline_color Optional color for the text outline (default:
  //! Transparent). Outline thickness is 0.f if set to Transparent.
  void render_text( std::string text, unsigned int size, sf::Vector2f position, Alignment align, float letter_spacing = 1.f,
                    sf::Color fill_color = sf::Color::White, sf::Color outline_color = sf::Color::Transparent );

  //! @brief Render a sprite by type/index to an arbitrary render target, drawing a fallback square if the sprite type or index is
  //! missing.
  //! @param target
  //! @param sprite_type
  //! @param pos_cmp
  //! @param sprite_index
  //! @param scale
  //! @param alpha
  //! @param origin
  //! @param angle
  void safe_render_sprite_to_target( sf::RenderTarget &target, const Sys::SpriteKey &sprite_type, const sf::FloatRect &pos_cmp,
                                     std::size_t sprite_index = 0, sf::Vector2f scale = { 1.f, 1.f }, uint8_t alpha = 255,
                                     sf::Vector2f origin = { 0.f, 0.f }, sf::Angle angle = sf::degrees( 0.f ) );

  //! @brief Draw a solid colored square to the given target in place of a missing sprite.
  //! @param target
  //! @param pos_cmp
  //! @param color
  void render_fallback_square_to_target( sf::RenderTarget &target, const sf::FloatRect &pos_cmp, const sf::Color &color = sf::Color::Magenta );

  //! @brief Render a sprite by type/index in screen view coordinates. Draws a fallback square if the sprite type or index is missing.
  //! @param sprite_type
  //! @param position
  //! @param sprite_index
  //! @param scale
  //! @param alpha
  //! @param origin
  //! @param angle
  void safe_render_sprite_screen( const Sys::SpriteKey &sprite_type, const sf::FloatRect &position, std::size_t sprite_index = 0,
                                  sf::Vector2f scale = { 1.f, 1.f }, uint8_t alpha = 255, sf::Vector2f origin = { 0.f, 0.f },
                                  sf::Angle angle = sf::degrees( 0.f ) );

  //! @brief Render a sprite by type/index in world view coordinates. Draws a fallback square if the sprite type or index is missing.
  //! @param sprite_type
  //! @param position
  //! @param sprite_index
  //! @param scale
  //! @param alpha
  //! @param origin
  //! @param angle
  void safe_render_sprite_world( const Sys::SpriteKey &sprite_type, const sf::FloatRect &position, std::size_t sprite_index = 0,
                                 sf::Vector2f scale = { 1.f, 1.f }, uint8_t alpha = 255, sf::Vector2f origin = { 0.f, 0.f },
                                 sf::Angle angle = sf::degrees( 0.f ) );

  //! @brief Draw a solid colored square in world view coordinates in place of a missing sprite.
  //! @param pos_cmp
  //! @param color
  void render_fallback_square_world( const sf::FloatRect &pos_cmp, const sf::Color &color = sf::Color::Magenta );

  //! @brief Draw an outlined rectangle for the given bounds in world view coordinates, if visible in the current world view.
  //! @param bounds
  //! @param color
  void render_rectbounds( Cmp::RectBounds &bounds, sf::Color color );

  //! @brief Draw an outlined square for every entity that has the given Component (used as a bounds rect), if visible in the world view.
  //! @tparam Component
  //! @param square_color
  //! @param square_thickness
  template <typename Component>
  void render_square_for_floatrect_cmp( sf::Color square_color = sf::Color::Red, float square_thickness = 1.f )
  {
    const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );
    for ( auto [entity, requested_cmp] : reg().view<Component>().each() )
    {
      if ( not Utils::is_visible_in_view( view_bounds, requested_cmp ) ) continue;
      sf::RectangleShape rectangle;
      rectangle.setSize( requested_cmp.size );
      rectangle.setPosition( requested_cmp.position );
      rectangle.setFillColor( sf::Color::Transparent );
      rectangle.setOutlineColor( square_color );
      rectangle.setOutlineThickness( square_thickness );
      draw_world( rectangle );
    }
  }

  //! @brief Current view of the game world.
  static sf::View s_world_view;

  //! @brief The z-order queue shared by the render systems. Refreshed each frame by RenderGameSystem::render_game(),
  //! so it is only valid to read after that call.
  static ZOrderQueue s_zorder_queue;

  //! @brief Default font for rendering text
  Cmp::Font m_font = Cmp::Font( "res/fonts/tuffy.ttf" );

  //! @brief The render target reference. Initialised to the sf::RenderWindow.
  std::reference_wrapper<sf::RenderTarget> m_current_target{ m_window };

  //! @brief Per-panel pool of persistent sf::Text objects backing TextColumn, keyed by TextColumn::cache_key.
  //! Keeps line count from one frame able to shrink/grow freely - unused trailing entries from a previous, longer frame
  //! are simply left undrawn rather than erased.
  std::unordered_map<std::string, std::vector<sf::Text>> m_text_column_cache;

  //! @brief Common window options for ImGui windows
  const int kImGuiWindowOptions = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_RENDERSYSTEM_HPP__
