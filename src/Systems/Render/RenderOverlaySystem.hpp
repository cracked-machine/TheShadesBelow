#ifndef SRC_SYSTEMS_RENDER_RENDEROVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_RENDEROVERLAYSYSTEM_HPP__

#include <Components/Position.hpp>
#include <Sprites/SpriteMetaType.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Render/UiData.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Optimizations.hpp>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>
#include <unordered_map>

// clang-format off
namespace Game::PathFinding { class SpatialHashGrid; enum class QueryCompass; } 
namespace Game::Cmp { class ZOrderValue; }
namespace Game::Cmp::Inventory { class WearLevel; }
namespace Game::Sprites { class SpriteSheet; class SpriteFactory; } 
namespace Game::PathFinding { class SpatialHashGrid; }
// clang-format on

namespace Game::Sys
{

//! @brief Renders everything layered on top of the game world: HUD elements (meters, labels, icons, texts), the shop and grimoire
//! overlays, and the debug overlay (misc stats, Z-order list, NPC list, entity inspector, navmesh/pathfinding visualisation).
class RenderOverlaySystem : public RenderSystem
{
public:
  //! @brief Construct a new Render Overlay System object. Loads the main, debug and shop UI layout data from their JSON files.
  RenderOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
      : RenderSystem( reg, window, sprite_factory, sound_bank ),
        m_debug_overlay_tex( { 1, 1 } )
  {
    m_main_ui_data = std::make_unique<Render::UiData>( "res/ui/ui.json" );
    m_dbg_ui_data = std::make_unique<Render::UiData>( "res/ui/dbg.json" );
    m_shop_ui_data = std::make_unique<Render::UiData>( "res/ui/shop.json" );
  };

  //! @brief Initialise the debug texture and use it to override the render target
  //!        Call this before using draw_screen()/draw_world()
  //! @param size
  void begin_debug_overlay( sf::Vector2u size )
  {
    if ( m_debug_overlay_tex.getSize() != size ) { [[maybe_unused]] auto _ = m_debug_overlay_tex.resize( size ); }
    m_debug_overlay_tex.clear( sf::Color::Transparent );
    set_render_target( m_debug_overlay_tex );
  }

  //! @brief reset the render target and finialise the debug texture
  //!        Call this after using draw_screen()/draw_world()
  void end_debug_overlay()
  {
    restore_render_target();
    m_debug_overlay_tex.display();
    m_debug_overlay_ready = true;
  }

  //! @brief Render the debug texture to the window.
  //!        Call this after using end_debug_overlay()
  //! @param window
  void draw_debug_overlay( sf::RenderWindow &window ) const
  {
    if ( m_debug_overlay_ready ) window.draw( sf::Sprite( m_debug_overlay_tex.getTexture() ) );
  }

  //! @brief init the weak pointer for the pathfinding navmesh
  //! @param spatial_grid_ptr
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &spatial_grid_ptr, const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr )
  {
    m_npc_navmesh = spatial_grid_ptr;
    m_reserved_sm = reserved_sm;
  }

  //! @brief Render the main UI's outline rectangles (e.g. panel borders).
  void render_ui_outlines();

  //! @brief Render the main UI's icons, excluding the inventory icon which is handled by render_ui_inventory_icon.
  void render_ui_icons();

  //! @brief Render the sprite in the Inventory UI.
  //! @note ParticleSprites are rendered seperately in RenderGameSystem::render_game
  void render_ui_inventory_icon();

  //! @brief Render the main UI's meters (health, fear, infamy, despair, toxicity, inventory wear) and flash their outline colour on
  //! recent changes.
  //! @param dt
  void render_ui_meters( sf::Time dt );

  //! @brief Render the main UI's static text elements.
  void render_ui_texts();

  //! @brief Render the main UI's dynamic labels (blast radius, cadaver count, wealth, current inventory item) and flash their colour on
  //! recent changes.
  //! @param dt
  void render_ui_labels( sf::Time dt );

  //! @brief Render the current dungeon level depth text while its display cooldown has not yet elapsed.
  void render_level_depth();

  //! @brief Render the shop UI overlay (outlines, item icons and slot labels) if a Cmp::Shop::Inventory is enabled in the registry.
  void render_shop_inventory_overlay();

  //! @brief Render the grimoire UI overlay listing spell entries and their shown/hidden state, if a Cmp::Grimoire is enabled in the
  //! registry.
  void render_grimoire_inventory_overlay();

  //! @brief Render the debug "entity_stats" panel: player/mouse position and direction, plus various entity counts.
  void render_ui_misc_stats();

  //! @brief Render the debug "zorder_list" panel: the Z-order and entity id of each queued render entry, excluding a fixed set of
  //! noisy sprite types.
  //! @param zorder_queue
  void render_ui_zorder_list( std::vector<ZOrder> &zorder_queue );

  //! @brief Render the debug "npc_list" panel listing each NPC's entity id, position and sprite type.
  void render_ui_npc_list();

  //! @brief Render the start/target squares of every NPC's in-progress lerp movement.
  void render_lerp_positions();

  //! @brief Draw an outlined square in world view coordinates.
  //! @param pos
  //! @param size
  //! @param color
  void render_square( sf::Vector2f pos, sf::Vector2f size, sf::Color color );

  //! @brief Render the countdown until the crypt maze next shuffles, if the shuffle timer has not already expired.
  //! @param pos
  //! @param size
  void render_crypt_maze_timer( sf::Vector2f pos, unsigned int size );

  //! @brief Draw a small wear-level bar above an item, filled proportionally to `wearlevel`.
  //! @param wearlevel
  //! @param pos
  void render_wear_level( float wearlevel, const Cmp::Position &pos );

  //! @brief Draw an outlined square around each spatial hash grid neighbour of `query_pos`, excluding the player and NPCs.
  //! @param query_pos
  //! @param color
  //! @param query_compass
  void render_spatial_grid_neighbours( const Cmp::Position &query_pos, sf::Color color, PathFinding::QueryCompass query_compass );

  //! @brief Draw an outlined square for each node of the A* path between `start_pos_cmp` and `end_pos_cmp`, if the start position is
  //! visible in the world view.
  //! @param start_pos_cmp
  //! @param end_pos_cmp
  //! @param color
  //! @param query_compass
  void render_pathfinding_vector( const Cmp::Position &start_pos_cmp, const Cmp::Position &end_pos_cmp, sf::Color color,
                                  PathFinding::QueryCompass query_compass );

  //! @brief Debug-draw the spatial hash grid navmesh: the neighbour bucket size and cell edges at every position in view.
  //! @param npc_navmesh
  void render_navmesh();

  //! @brief Render the debug "inspect_list" panel: a breakdown of the components and stats of the entity currently under the mouse
  //! cursor.
  void render_ui_entity_inspect();

  //! @brief Draw an outlined square over the entity's Cmp::Position, if it also has the given Component.
  //! @tparam Component
  //! @param entity
  //! @param square_color
  //! @param square_thickness
  template <typename Component>
  void render_square_for_entity( entt::entity entity, sf::Color square_color = sf::Color::Red, float square_thickness = 1.f )
  {
    auto *pos_cmp = reg().try_get<Cmp::Position>( entity );
    auto requested_cmp = reg().try_get<Component>( entity );
    if ( pos_cmp && requested_cmp )
    {
      sf::RectangleShape rectangle;
      rectangle.setSize( Constants::kGridSizePxF );
      rectangle.setPosition( pos_cmp->position );
      rectangle.setFillColor( sf::Color::Transparent );
      rectangle.setOutlineColor( square_color );
      rectangle.setOutlineThickness( square_thickness );
      draw_world( rectangle );
    }
  }

  //! @brief Draw an outlined square for every entity that has the given Component (used as a position), if visible in the world view.
  //! @tparam Component
  //! @param square_color
  //! @param square_thickness
  template <typename Component>
  void render_square_for_vector2f_cmp( sf::Color square_color = sf::Color::Red, float square_thickness = 1.f )
  {
    const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );
    for ( auto [entity, requested_cmp] : reg().view<Component>().each() )
    {
      if ( not Utils::is_visible_in_view( view_bounds, sf::FloatRect( requested_cmp, Constants::kGridSizePxF ) ) ) continue;
      sf::RectangleShape rectangle;
      rectangle.setSize( Constants::kGridSizePxF );
      rectangle.setPosition( requested_cmp );
      rectangle.setFillColor( sf::Color::Transparent );
      rectangle.setOutlineColor( square_color );
      rectangle.setOutlineThickness( square_thickness );
      draw_world( rectangle );
    }
  }

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

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief Advances `interval` by `dt` while a `FlashComponent` is present on any entity, removing it once its duration has elapsed.
  //! Returns whether the UI element should currently be drawn in its "flashed" state (toggles on/off at m_ui_flash_factor ms).
  //! @tparam FlashComponent
  //! @param dt
  //! @param interval accumulated flash time; reset to zero once the flash expires
  //! @return true if the UI element should currently be drawn in its "flashed" state, false otherwise.
  template <typename FlashComponent>
  bool update_flash_toggle( sf::Time dt, sf::Time &interval )
  {
    auto view = reg().view<FlashComponent>();
    if ( view.empty() ) return false;

    auto flash_entt = view.front();
    auto &flash_cmp = view.template get<FlashComponent>( flash_entt );
    interval += dt;
    if ( flash_cmp.duration != sf::Time::Zero and interval > flash_cmp.duration )
    {
      reg().remove<FlashComponent>( flash_entt );
      interval = sf::Time::Zero;
      return false;
    }
    return static_cast<int>( interval.asMilliseconds() / m_ui_flash_factor ) % 2 == 1;
  }

  //! @brief Draw a column of debug text lines, top to bottom, at a fixed origin, advancing by `line_height` after each call.
  //! Backed by a per-`cache_key` pool of persistent sf::Text objects (see m_debug_text_cache) so that, across frames, each
  //! line reuses the same sf::Text instead of being reconstructed (and having its outline re-generated) from scratch -
  //! these panels can otherwise update every frame at a real cost to frame time.
  struct DebugTextColumn
  {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members) - DebugTextColumn is a short-lived,
    // per-panel-call local (never stored, copy-assigned, or passed around), so the usual dangling-reference
    // risk this check guards against doesn't apply here.
    //! @brief The overlay system used to draw each text line.
    RenderOverlaySystem &self;

    //! @brief Identifies this column's slot in self.m_debug_text_cache. Stable across frames for a given panel (e.g.
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
      auto &pool = self.m_debug_text_cache[cache_key];
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

  //! @brief Draw a list of UI outline rectangles (panel borders).
  //! @param outlines
  void render_outlines( const std::vector<Render::UiData::Outline> &outlines )
  {
    sf::RectangleShape rect;
    for ( const auto &outline : outlines )
    {
      rect.setSize( outline.rect.size );
      rect.setPosition( outline.rect.position );
      rect.setFillColor( outline.fill_color );
      rect.setOutlineColor( outline.line_color );
      rect.setOutlineThickness( static_cast<float>( outline.line_thickness ) );
      draw_screen( rect );
    }
  }

  //! @brief Layout data object for the main UI
  std::unique_ptr<Render::UiData> m_main_ui_data;

  //! @brief Layout data object for the debug UI
  std::unique_ptr<Render::UiData> m_dbg_ui_data;

  //! @brief Layout data object for the shop scene overlay
  std::unique_ptr<Render::UiData> m_shop_ui_data;

private:
  //! @brief Draw the debug ui to this texture.
  sf::RenderTexture m_debug_overlay_tex;

  //! @brief Signals when the texture is ready to be drawn to the sf::RenderWindow
  bool m_debug_overlay_ready{ false };

  //! @brief tracks the npc pathfinding navmesh i.e. where the NPC cannot move to
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;

  //! @brief Per-panel pool of persistent sf::Text objects backing DebugTextColumn, keyed by DebugTextColumn::cache_key.
  //! Keeps line count from one frame able to shrink/grow freely - unused trailing entries from a previous, longer frame
  //! are simply left undrawn rather than erased.
  std::unordered_map<std::string, std::vector<sf::Text>> m_debug_text_cache;

  //! @brief Used to flash the UI wealth text
  sf::Time m_flash_wealth_ui_interval;

  //! @brief Used to flash the UI health text
  sf::Time m_flash_health_ui_interval;

  //! @brief Used to flash the UI cadaver text
  sf::Time m_flash_cadaver_ui_interval;

  //! @brief Used to flash the UI inevntory text
  sf::Time m_flash_inventory_ui_interval;

  //! @brief Used to flash the UI radius text
  sf::Time m_flash_radius_ui_interval;

  //! @brief Screen flash frequency
  int m_ui_flash_factor{ 300 };
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_RENDEROVERLAYSYSTEM_HPP__
