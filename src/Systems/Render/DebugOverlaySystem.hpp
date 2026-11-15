#ifndef SRC_SYSTEMS_RENDER_DEBUGOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_DEBUGOVERLAYSYSTEM_HPP__

#include <Components/Position.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/Render/BaseOverlaySystem.hpp>
#include <Systems/Render/UiLayout.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Optimizations.hpp>
#include <span>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <memory>
#include <string_view>

// clang-format off
namespace Game::PathFinding { enum class QueryCompass; }
// clang-format on

namespace Game::Sys
{

//! @brief Renders the debug visualisations toggled by the Cmp::SceneSettings::ShowNavmesh, ShowPathFinding and ShowDebugStats scene
//! settings: the navmesh, NPC pathfinding, and the debug overlay (misc stats, Z-order list, NPC list, entity inspector).
//! OverlayArbitrationSystem calls update() after RenderGameSystem::render_game().
class DebugOverlaySystem : public BaseOverlaySystem
{
public:
  //! @brief Construct a new Debug Overlay System object. Loads the debug UI layout data from its JSON file.
  DebugOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointer for the pathfinding navmesh
  //! @param spatial_grid_ptr
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &spatial_grid_ptr, const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr )
  {
    m_npc_navmesh = spatial_grid_ptr;
    m_reserved_sm = reserved_sm;
  }

  //! @brief Render whichever debug visualisations are enabled in the current scene settings. The debug panels are only
  //! shown while OverlayArbitrationSystem::debug_panels_visible().
  //! Must be called after RenderGameSystem::render_game() in the same frame, as the debug stats overlay lists
  //! the z-order queue (`s_zorder_queue`) that call refreshes.
  //! @param dt
  void update( sf::Time dt ) override;

private:
  //! @brief Debug-draw the spatial hash grid navmesh: the neighbour bucket size and cell edges at every position in view.
  void render_navmesh();

  //! @brief Draw the player/NPC spatial grid neighbours, NPC lerp positions and the A* path from each NPC to the player.
  void render_pathfinding();

  //! @brief Draw the crypt room outlines, selected positions, particle emitter positions and the half-view bounds, then the
  //! debug overlay panels (misc stats, Z-order list, NPC list, entity inspector) as semi-transparent ImGui windows on top.
  void render_debug_stats();

  //! @brief Begin one of the debug overlay's ImGui windows, placed at the matching "res/ui/dbg.json" label position on first use.
  //!        Must always be paired with ImGui::End(), whatever this returns.
  //! @param title The ImGui window title (also its persisted-layout id)
  //! @param label_name The dbg.json ui_label that gives the window's initial position
  //! @param initial_size The window's initial size in pixels; the height is clamped to fit below the window's position
  //! @param start_collapsed Start with only the title bar showing. Like the position and size, this only applies on first use;
  //!        after that the collapsed state saved in res/imgui.ini wins.
  //! @return true if the window is expanded and its contents should be submitted
  bool begin_debug_window( const char *title, std::string_view label_name, sf::Vector2f initial_size, bool start_collapsed = false );

  //! @brief Render the debug "Highlights" ImGui window: one checkbox (with a swatch of its colour) per debug rectangle type
  //! drawn by render_debug_stats(). Must be called before those rectangles are drawn so a toggle applies in the same frame.
  void render_ui_highlight_options();

  //! @brief Render the debug "Stats" ImGui window: player/mouse position and direction, plus various entity counts.
  void render_ui_misc_stats();

  //! @brief Render the debug "Z Order" ImGui window as a scrollable, clipped list: the Z-order and entity id of each queued render entry, excluding a fixed set of
  //! noisy sprite types.
  void render_ui_zorder_list();

  //! @brief Render the debug "NPCs" ImGui window as a scrollable, clipped list of each NPC's entity id, position and sprite type.
  void render_ui_npc_list();

  //! @brief Render the debug "Inspect" ImGui window: a breakdown of the components and stats of the entity currently under the mouse
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
  std::unique_ptr<Render::UiLayout> m_dbg_ui_layout;

  //! @brief Which debug rectangles render_debug_stats() draws, toggled from the "Highlights" window. All off by default.
  struct HighlightToggles
  {
    bool lava_pit_cells{ false };
    bool open_rooms{ false };
    bool start_room{ false };
    bool end_room{ false };
    bool closed_rooms{ false };
    bool passage_blocks{ false };
    bool selected_positions{ false };
    bool half_view_bounds{ false };
    bool grave_npc_traps{ false };
    bool grave_trail_markers{ false };
    bool grave_trail_start{ false };
  };

  //! @brief The current debug rectangle toggles.
  HighlightToggles m_highlights;

  //! @brief tracks the npc pathfinding navmesh i.e. where the NPC cannot move to
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_DEBUGOVERLAYSYSTEM_HPP__
