#ifndef SRC_SYSTEMS_RENDER_RENDERDEBUGSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_RENDERDEBUGSYSTEM_HPP__

#include <Components/Position.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Render/UiData.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Optimizations.hpp>
#include <span>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>

#include <memory>
#include <vector>

// clang-format off
namespace Game::PathFinding { enum class QueryCompass; }
// clang-format on

namespace Game::Sys
{

//! @brief Renders the debug visualisations toggled by the Cmp::SceneSettings::ShowNavmesh, ShowPathFinding and ShowDebugStats scene
//! settings: the navmesh, NPC pathfinding, and the debug overlay (misc stats, Z-order list, NPC list, entity inspector).
//! Scenes call render_debug() after RenderGameSystem::render_game().
class RenderDebugSystem : public RenderSystem
{
public:
  //! @brief Construct a new Render Debug System object. Loads the debug UI layout data from its JSON file.
  RenderDebugSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointer for the pathfinding navmesh
  //! @param spatial_grid_ptr
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &spatial_grid_ptr, const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr )
  {
    m_npc_navmesh = spatial_grid_ptr;
    m_reserved_sm = reserved_sm;
  }

  //! @brief Entrypoint for rendering whichever debug visualisations are enabled in the current scene settings.
  //! Must be called after RenderGameSystem::render_game() in the same frame, as the debug stats overlay lists
  //! the z-order queue (`s_zorder_queue`) that call refreshes.
  void render_debug();

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Debug-draw the spatial hash grid navmesh: the neighbour bucket size and cell edges at every position in view.
  void render_navmesh();

  //! @brief Draw the player/NPC spatial grid neighbours, NPC lerp positions and the A* path from each NPC to the player.
  void render_pathfinding();

  //! @brief Draw the crypt room outlines and the debug overlay panels (misc stats, Z-order list, NPC list, entity inspector),
  //! plus selected positions, particle emitter positions and the half-view bounds.
  void render_debug_stats();

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
  void draw_debug_overlay() const
  {
    if ( m_debug_overlay_ready ) m_window.draw( sf::Sprite( m_debug_overlay_tex.getTexture() ) );
  }

  //! @brief Render the debug "entity_stats" panel: player/mouse position and direction, plus various entity counts.
  void render_ui_misc_stats();

  //! @brief Render the debug "zorder_list" panel: the Z-order and entity id of each queued render entry, excluding a fixed set of
  //! noisy sprite types.
  void render_ui_zorder_list();

  //! @brief Render the debug "npc_list" panel listing each NPC's entity id, position and sprite type.
  void render_ui_npc_list();

  //! @brief Render the debug "inspect_list" panel: a breakdown of the components and stats of the entity currently under the mouse
  //! cursor.
  void render_ui_entity_inspect();

  //! @brief Render the start/target squares of every NPC's in-progress lerp movement.
  void render_lerp_positions();

  //! @brief Draw an outlined square in world view coordinates.
  //! @param pos
  //! @param size
  //! @param color
  void render_square( sf::Vector2f pos, sf::Vector2f size, sf::Color color );

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
  //! @param footprint_cells The cells an NPC stands on, anchor cell first, see Cmp::Npc::Footprint.
  void render_pathfinding_vector( const Cmp::Position &start_pos_cmp, const Cmp::Position &end_pos_cmp, sf::Color color,
                                  PathFinding::QueryCompass query_compass, std::span<const sf::Vector2i> footprint_cells = {} );

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

  //! @brief Layout data object for the debug UI
  std::unique_ptr<Render::UiData> m_dbg_ui_data;

  //! @brief Draw the debug ui to this texture.
  sf::RenderTexture m_debug_overlay_tex;

  //! @brief Signals when the texture is ready to be drawn to the sf::RenderWindow
  bool m_debug_overlay_ready{ false };

  //! @brief tracks the npc pathfinding navmesh i.e. where the NPC cannot move to
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_RENDERDEBUGSYSTEM_HPP__
