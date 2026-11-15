#include <Components/Altar/MultiBlock.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/Armed.hpp>
#include <Components/Crypt/BuildingMultiBlock.hpp>
#include <Components/Crypt/BuildingSegment.hpp>
#include <Components/Crypt/Entrance.hpp>
#include <Components/Crypt/PassageBlock.hpp>
#include <Components/Crypt/PassageDoor.hpp>
#include <Components/Crypt/RoomClosed.hpp>
#include <Components/Crypt/RoomEnd.hpp>
#include <Components/Crypt/RoomLavaPit.hpp>
#include <Components/Crypt/RoomLavaPitCell.hpp>
#include <Components/Crypt/RoomOpen.hpp>
#include <Components/Crypt/RoomStart.hpp>
#include <Components/Direction.hpp>
#include <Components/Exit.hpp>
#include <Components/FootStepTimer.hpp>
#include <Components/Grave/Consequence.hpp>
#include <Components/Grave/ExitSegment.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Grave/Segment.hpp>
#include <Components/Grave/TrailMarker.hpp>
#include <Components/Grave/TrailStart.hpp>
#include <Components/Hazard/CorruptionCell.hpp>
#include <Components/Hazard/SinkholeCell.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LastDirection.hpp>
#include <Components/LerpPosition.hpp>
#include <Components/Moveable.hpp>
#include <Components/Npc/Footprint.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/SpawnedBy.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/Illuminated.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Player/PendingNoPath.hpp>
#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Ruin/BuildingMultiBlock.hpp>
#include <Components/Ruin/BuildingSegment.hpp>
#include <Components/Ruin/Cobweb.hpp>
#include <Components/Ruin/Entrance.hpp>
#include <Components/SceneSettings/ShowDebugStats.hpp>
#include <Components/SceneSettings/ShowNavmesh.hpp>
#include <Components/SceneSettings/ShowPathFinding.hpp>
#include <Components/SelectedPosition.hpp>
#include <Components/Spring/HealingSpringBuildingMultiBlock.hpp>
#include <Components/Spring/HealingSpringBuildingSegment.hpp>
#include <Components/Spring/HealingSpringEntrance.hpp>
#include <Components/Wall.hpp>
#include <Components/ZOrderValue.hpp>
#include <PathFinding/AStar.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/Render/DebugOverlaySystem.hpp>
#include <Systems/Render/OverlayArbitrationSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Profiling.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <tracy/Tracy.hpp>
#include <vector>

namespace Game::Sys
{

namespace
{

//! @brief Background opacity of the debug overlay windows: low enough to see the game through, high enough to read the text.
constexpr float kDebugBgAlpha = 0.3f;

//! @brief Colours of the debug rectangles, shared by the draw calls and the "Highlights" window swatches.
constexpr sf::Color kLavaPitCellColour{ 254, 128, 32 };
constexpr sf::Color kOpenRoomColour = sf::Color::Green;
constexpr sf::Color kStartRoomColour = sf::Color::Blue;
constexpr sf::Color kEndRoomColour = sf::Color::Yellow;
constexpr sf::Color kClosedRoomColour = sf::Color::Red;
constexpr sf::Color kPassageBlockColour = sf::Color::Black;
constexpr sf::Color kSelectedPositionColour = sf::Color::Yellow;
constexpr sf::Color kHalfViewColour = sf::Color::Red;
constexpr sf::Color kGraveNpcTrapColour = sf::Color::Blue;
constexpr sf::Color kGraveTrailMarkerColour = sf::Color::Green;
constexpr sf::Color kGraveTrailStartColour = sf::Color::Yellow;

//! @brief Convert an sf::Color to an ImGui colour.
ImVec4 to_imvec4( sf::Color c ) { return { c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f }; }

//! @brief Draw one line of unformatted text in the given colour (TextUnformatted, so a '%' in a name is never treated as a format).
void text_line( const std::string &str, sf::Color colour = sf::Color::White )
{
  ImGui::PushStyleColor( ImGuiCol_Text, to_imvec4( colour ) );
  ImGui::TextUnformatted( str.c_str() );
  ImGui::PopStyleColor();
}

} // namespace

DebugOverlaySystem::DebugOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseOverlaySystem( reg, window, sound_bank ),
      m_dbg_ui_layout( std::make_unique<Render::UiLayout>( "res/ui/dbg.json" ) )
{
  SPDLOG_DEBUG( "DebugOverlaySystem initialized" );
}

void DebugOverlaySystem::update( [[maybe_unused]] sf::Time dt )
{
  if ( Utils::scene_setting<Cmp::SceneSettings::ShowNavmesh>( reg() ).enabled ) { PROFILED( render_navmesh() ); }
  if ( Utils::scene_setting<Cmp::SceneSettings::ShowPathFinding>( reg() ).enabled ) { PROFILED( render_pathfinding() ); }
  if ( OverlayArbitrationSystem::debug_panels_visible( reg() ) ) { PROFILED( render_debug_stats() ); }
}

void DebugOverlaySystem::render_navmesh()
{
  const auto npc_navmesh = m_npc_navmesh.lock();
  if ( not npc_navmesh ) return;

  sf::Text text( m_font, "", 10 );
  for ( auto [pos_entt, pos_cmp] : reg().view<Cmp::Position>().each() )
  {
    if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), pos_cmp ) ) continue;
    auto entt_bucket = npc_navmesh->at( pos_cmp );
    text.setString( std::to_string( entt_bucket.size() ) );
    text.setFillColor( sf::Color::Blue );
    text.setOutlineColor( sf::Color::Black );
    text.setOutlineThickness( 1.f );
    text.setPosition( { pos_cmp.position.x + 4.f, pos_cmp.position.y + 4.f } );

    draw_world( text );

    sf::RectangleShape bottom_edge( { 8.f, 1.f } );
    bottom_edge.setFillColor( sf::Color::Blue );
    bottom_edge.setOutlineThickness( 0.f );
    bottom_edge.setPosition( { pos_cmp.position.x + ( pos_cmp.size.x / 2 ), pos_cmp.position.y + pos_cmp.size.y } );
    draw_world( bottom_edge );

    sf::RectangleShape right_edge( { 1.f, 8.f } );
    right_edge.setFillColor( sf::Color::Blue );
    right_edge.setOutlineThickness( 0.f );
    right_edge.setPosition( { pos_cmp.position.x + pos_cmp.size.x, pos_cmp.position.y + ( pos_cmp.size.y / 2 ) } );
    draw_world( right_edge );
  }
}

void DebugOverlaySystem::render_pathfinding()
{
  const Cmp::Position player_pos_cmp = Utils::Player::get_position( reg() );
  Cmp::Position player_center_hitbox( player_pos_cmp.getCenter(), { 1.f, 1.f } );
  render_square( player_center_hitbox.position, player_center_hitbox.size, sf::Color::Blue );
  render_lerp_positions();
  render_spatial_grid_neighbours( player_center_hitbox, sf::Color::Cyan, PathFinding::QueryCompass::CARDINAL );

  for ( auto [npc_entt, npc_cmp, npc_pos_cmp, anim_cmp, footprint_cmp] :
        reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::AnimData, Cmp::Npc::Footprint>().each() )
  {
    auto query_compass = PathFinding::QueryCompass::CARDINAL;
    if ( anim_cmp.m_sprite_type.contains( "sprite.ghost" ) ) query_compass = PathFinding::QueryCompass::BOTH;
    Cmp::Position npc_center_hitbox( npc_pos_cmp.getCenter(), { 1.f, 1.f } );

    render_spatial_grid_neighbours( npc_center_hitbox, sf::Color::Magenta, query_compass );
    // mirror Utils::Npc::pathfind_toward: the path is for the cell(s) the NPC stands on, not its top-left
    render_pathfinding_vector( footprint_cmp.anchor( npc_pos_cmp.position ), player_pos_cmp, sf::Color::White, query_compass, footprint_cmp.cells() );
  }
}

void DebugOverlaySystem::render_debug_stats()
{
  // these debug shapes are only drawn within the current view to prevent FPS drops
  ZoneScopedN( "RenderDebugUI" );

  // The debug panels are ImGui windows, drawn on top of everything else when OverlayArbitrationSystem renders the shared
  // ImGui frame.

  // Transparent child/list/title backgrounds, so only the window's own (SetNextWindowBgAlpha) background shows through.
  constexpr std::array kTransparentCols{ ImGuiCol_ChildBg, ImGuiCol_FrameBg, ImGuiCol_TitleBg, ImGuiCol_TitleBgActive, ImGuiCol_TitleBgCollapsed };
  for ( auto col : kTransparentCols )
  {
    ImGui::PushStyleColor( col, ImVec4( 0.f, 0.f, 0.f, 0.f ) );
  }

  // before the rectangles below, so a checkbox change applies this frame (ImGui only draws when the shared frame is rendered)
  PROFILED( render_ui_highlight_options() );

  if ( m_highlights.lava_pit_cells ) render_square_for_floatrect_cmp<Cmp::Crypt::RoomLavaPitCell>( kLavaPitCellColour, 0.5f );
  if ( m_highlights.open_rooms ) render_square_for_floatrect_cmp<Cmp::Crypt::RoomOpen>( kOpenRoomColour, 1.f );
  if ( m_highlights.start_room ) render_square_for_floatrect_cmp<Cmp::Crypt::RoomStart>( kStartRoomColour, 1.f );
  if ( m_highlights.end_room ) render_square_for_floatrect_cmp<Cmp::Crypt::RoomEnd>( kEndRoomColour, 1.f );
  if ( m_highlights.closed_rooms ) render_square_for_floatrect_cmp<Cmp::Crypt::RoomClosed>( kClosedRoomColour, 1.f );
  if ( m_highlights.passage_blocks ) render_square_for_vector2f_cmp<Cmp::Crypt::PassageBlock>( kPassageBlockColour, 1.f );

  if ( m_highlights.selected_positions )
  {
    for ( auto [selected_entt, selected_cmp, pos_cmp] : reg().view<Cmp::SelectedPosition, Cmp::Position>().each() )
    {
      if ( not Utils::is_visible_in_view( RenderSystem::get_world_view(), pos_cmp ) ) continue;
      PROFILED( render_square( pos_cmp.position, pos_cmp.size, kSelectedPositionColour ) );
    }
  }

  for ( auto [ps_owner_entt, ps_owner_cmp] : reg().view<Cmp::Particle::SpriteOwner>().each() )
  {
    auto emitter_pos = ps_owner_cmp.sprite->get_emitter_position();
    auto dot = sf::CircleShape( 1 );
    dot.setPosition( emitter_pos );
    dot.setFillColor( sf::Color::Cyan );
    dot.setOutlineColor( sf::Color::Cyan );
    draw_world( dot );
  }

  if ( m_highlights.half_view_bounds )
  {
    auto half_view = Cmp::RectBounds::scaled( Utils::calculate_view_bounds( Sys::RenderSystem::get_world_view() ), 0.5f );
    render_square( half_view.getBounds().position, half_view.getBounds().size, kHalfViewColour );
  }

  if ( m_highlights.grave_npc_traps )
  {
    const int player_luck = Utils::Player::get_stats( reg() ).luck();
    for ( auto [grave_entt, grave_mb, consequence] : reg().view<Cmp::Grave::MultiBlock, Cmp::Grave::Consequence>().each() )
    {
      if ( not Utils::is_visible_in_view( RenderSystem::get_world_view(), grave_mb ) ) continue;
      if ( consequence.roll( player_luck ) != Cmp::Grave::Consequence::Type::NPC_TRAP ) continue;
      render_square( grave_mb.position, grave_mb.size, kGraveNpcTrapColour );
    }
  }

  if ( m_highlights.grave_trail_markers )
  {
    for ( auto [grave_entt, grave_mb, trail_cmp] : reg().view<Cmp::Grave::MultiBlock, Cmp::Grave::TrailMarker>().each() )
    {
      if ( not Utils::is_visible_in_view( RenderSystem::get_world_view(), grave_mb ) ) continue;
      render_square( grave_mb.position, grave_mb.size, kGraveTrailMarkerColour );
    }
  }

  if ( m_highlights.grave_trail_start )
  {
    for ( auto [grave_entt, grave_mb] : reg().view<Cmp::Grave::MultiBlock, Cmp::Grave::TrailStart>().each() )
    {
      if ( not Utils::is_visible_in_view( RenderSystem::get_world_view(), grave_mb ) ) continue;
      render_square( grave_mb.position, grave_mb.size, kGraveTrailStartColour );
    }
  }

  PROFILED( render_ui_misc_stats() );
  PROFILED( render_ui_zorder_list() );
  PROFILED( render_ui_npc_list() );
  PROFILED( render_ui_entity_inspect() );

  ImGui::PopStyleColor( static_cast<int>( kTransparentCols.size() ) );
}

bool DebugOverlaySystem::begin_debug_window( const char *title, std::string_view label_name, sf::Vector2f initial_size, bool start_collapsed )
{
  const sf::Vector2f window_size( m_window.getSize() );
  sf::Vector2f pos{ 0.f, 0.f };
  if ( m_dbg_ui_layout )
  {
    auto it = std::ranges::find_if( m_dbg_ui_layout->m_labels, [&]( const auto &label ) { return label.name == label_name; } );
    if ( it != m_dbg_ui_layout->m_labels.end() )
    {
      // debug windows are free-floating, so stretch the design-space position per axis (keeps edge-anchored panels on the edges)
      const sf::Vector2f design = m_dbg_ui_layout->design_size();
      pos = { it->rect.position.x * window_size.x / design.x, it->rect.position.y * window_size.y / design.y };
    }
  }
  // fit the initial size inside the window from that position, so the panel's scrollbar is reachable at any resolution
  const sf::Vector2f size{ std::min( initial_size.x, window_size.x - pos.x ), std::min( initial_size.y, window_size.y - pos.y ) };

  ImGui::SetNextWindowPos( ImVec2( pos.x, pos.y ), ImGuiCond_FirstUseEver );
  ImGui::SetNextWindowSize( ImVec2( size.x, size.y ), ImGuiCond_FirstUseEver );
  ImGui::SetNextWindowCollapsed( start_collapsed, ImGuiCond_FirstUseEver );
  ImGui::SetNextWindowBgAlpha( kDebugBgAlpha );
  return ImGui::Begin( title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing );
}

void DebugOverlaySystem::render_ui_highlight_options()
{
  if ( begin_debug_window( "Highlights", "highlight_options", /*initial_size=*/{ 300.f, 300.f }, /*start_collapsed=*/true ) )
  {
    auto row = []( const char *label, bool &toggle, sf::Color colour )
    {
      ImGui::PushID( label );
      ImGui::ColorButton( "##swatch", to_imvec4( colour ), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker,
                          ImVec2( ImGui::GetFrameHeight(), ImGui::GetFrameHeight() ) );
      ImGui::PopID();
      ImGui::SameLine();
      ImGui::Checkbox( label, &toggle );
    };

    // checkbox squares use FrameBg, which render_debug_stats() makes transparent for the list panels
    ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.2f, 0.2f, 0.2f, 0.8f ) );

    ImGui::SeparatorText( "Crypt" );
    row( "Lava pit cells", m_highlights.lava_pit_cells, kLavaPitCellColour );
    row( "Open rooms", m_highlights.open_rooms, kOpenRoomColour );
    row( "Start room", m_highlights.start_room, kStartRoomColour );
    row( "End room", m_highlights.end_room, kEndRoomColour );
    row( "Closed rooms", m_highlights.closed_rooms, kClosedRoomColour );
    row( "Passage blocks", m_highlights.passage_blocks, kPassageBlockColour );

    ImGui::SeparatorText( "Graveyard" );
    row( "Grave NPC traps", m_highlights.grave_npc_traps, kGraveNpcTrapColour );
    row( "Grave trail markers", m_highlights.grave_trail_markers, kGraveTrailMarkerColour );
    row( "Grave trail start", m_highlights.grave_trail_start, kGraveTrailStartColour );

    ImGui::SeparatorText( "General" );
    row( "Selected positions", m_highlights.selected_positions, kSelectedPositionColour );
    row( "Half view bounds", m_highlights.half_view_bounds, kHalfViewColour );

    ImGui::PopStyleColor();
  }
  ImGui::End();
}

void DebugOverlaySystem::render_ui_misc_stats()
{
  if ( begin_debug_window( "Stats", "entity_stats", /*initial_size=*/{ 460.f, 260.f } ) )
  {
    const auto player_pos = Utils::Player::get_position( reg() );
    ImGui::Text( "Player position - [%d , %d]", static_cast<int>( player_pos.x() ), static_cast<int>( player_pos.y() ) );

    sf::Vector2i mouse_pixel_pos = sf::Mouse::getPosition( m_window );
    sf::Vector2f mouse_world_pos = m_window.mapPixelToCoords( mouse_pixel_pos, RenderSystem::get_world_view() );
    ImGui::Text( "Mouse position - [%d , %d]", static_cast<int>( mouse_world_pos.x ), static_cast<int>( mouse_world_pos.y ) );

    const auto dir = Utils::Player::get_direction( reg() );
    const auto last_dir = Utils::Player::get_last_direction( reg() );
    ImGui::Text( "Player current direction - [%.2f , %.2f]", static_cast<double>( dir.x ), static_cast<double>( dir.y ) );
    ImGui::Text( "Player last direction - [%.2f , %.2f]", static_cast<double>( last_dir.x ), static_cast<double>( last_dir.y ) );

    ImGui::Separator();
    ImGui::Text( "Entities - %u", static_cast<unsigned>( reg().view<entt::entity>().size() ) );
    ImGui::Text( "NPCs - %u", static_cast<unsigned>( reg().view<Cmp::Npc::NPC>().size() ) );
    ImGui::Text( "Corruption - %u", static_cast<unsigned>( reg().view<Cmp::CorruptionCell>().size() ) );
    ImGui::Text( "Sinkhole - %u", static_cast<unsigned>( reg().view<Cmp::SinkholeCell>().size() ) );
    ImGui::Text( "CryptPassageBlocks - %u", static_cast<unsigned>( reg().view<Cmp::Crypt::PassageBlock>().size() ) );
  }
  ImGui::End();
}

void DebugOverlaySystem::render_ui_zorder_list()
{
  // clang-format off
  static const std::set<std::string, std::less<>> kExclusions = {
      "sprite.graveyard.wall.ext",
      "sprite.graveyard.wall.int.main",
      "sprite.graveyard.wall.int.cap",
      "sprite.ruin.wall.int",
      "sprite.well.wall.int",
      "sprite.shop.wall.int",
      "sprite.ruin.wall.ext",
      "sprite.crypt.wall.ext",
      "sprite.graveyard.playerspawn",
      "sprite.skeleton",
      "sprite.ghost",
      "sprite.wisp",
      "sprite.graveyard.detonated",
      "sprite.player.footsteps",
      "sprite.ruin.cobweb",
      "sprite.ruin.bookcase.left",
      "sprite.ruin.bookcase.mid",
      "sprite.ruin.bookcase.right"
  };
  // clang-format on

  struct Row
  {
    int z;
    uint32_t e;
    std::string name;
  };

  if ( begin_debug_window( "Z Order", "zorder_list", /*initial_size=*/{ 520.f, 600.f }, /*start_collapsed=*/true ) )
  {
    std::vector<Row> rows;
    rows.reserve( s_zorder_queue.size() );
    for ( const auto &zorder_entry : s_zorder_queue )
    {
      std::string name;
      if ( auto *sprite_anim_cmp = reg().try_get<Cmp::AnimData>( zorder_entry.e ) ) { name = sprite_anim_cmp->m_sprite_type.str(); }
      if ( auto *particle_sprite_owner = reg().try_get<Cmp::Particle::SpriteOwner>( zorder_entry.e ) )
      {
        name = particle_sprite_owner->sprite->get_tag();
      }
      if ( name.empty() or kExclusions.contains( name ) ) continue;
      rows.push_back( { static_cast<int>( zorder_entry.z ), static_cast<uint32_t>( zorder_entry.e ), std::move( name ) } );
    }

    ImGui::Text( "%u entries", static_cast<unsigned>( rows.size() ) );
    ImGui::Separator();
    if ( ImGui::BeginChild( "zorder_rows" ) )
    {
      // only the rows inside the scrolled region are submitted
      ImGuiListClipper clipper;
      clipper.Begin( static_cast<int>( rows.size() ) );
      while ( clipper.Step() )
      {
        for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i )
        {
          const auto &row = rows[static_cast<std::size_t>( i )];
          ImGui::Text( "Z:%d E:%u - %s", row.z, row.e, row.name.c_str() );
        }
      }
    }
    ImGui::EndChild();
  }
  ImGui::End();
}

void DebugOverlaySystem::render_ui_npc_list()
{
  if ( begin_debug_window( "NPCs", "npc_list", /*initial_size=*/{ 300.f, 400.f }, /*start_collapsed=*/true ) )
  {
    auto npc_view = reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::AnimData>();
    ImGui::Text( "%u NPCs", static_cast<unsigned>( npc_view.size_hint() ) );
    ImGui::Separator();
    if ( ImGui::BeginChild( "npc_rows" ) )
    {
      for ( auto [npc_entity, npc_cmp, npc_pos_cmp, npc_anim_cmp] : npc_view.each() )
      {
        ImGui::Text( "%u: [%d,%d] - %s", entt::to_integral( npc_entity ), static_cast<int>( npc_pos_cmp.position.x ),
                     static_cast<int>( npc_pos_cmp.position.y ), npc_cmp.npc_type.str().c_str() );
      }
    }
    ImGui::EndChild();
  }
  ImGui::End();
}

void DebugOverlaySystem::render_ui_entity_inspect()
{
  if ( not begin_debug_window( "Inspect", "inspect_list", { 460.f, 500.f } ) )
  {
    ImGui::End();
    return;
  }

  sf::Vector2i mouse_pixel_pos = sf::Mouse::getPosition( m_window );
  sf::Vector2f mouse_world_pos = m_window.mapPixelToCoords( mouse_pixel_pos, RenderSystem::get_world_view() );

  // hovering a debug window would otherwise inspect whatever world tile happens to be under it
  const bool over_debug_ui = ImGui::GetIO().WantCaptureMouse;

  auto position_view = reg().view<Cmp::Position>();
  for ( auto [entity, pos_cmp] : position_view.each() )
  {
    if ( over_debug_ui ) break;
    if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), pos_cmp ) ) continue;
    if ( not pos_cmp.contains( mouse_world_pos ) ) { continue; }

    text_line( "--- Entity #" + std::to_string( entt::to_integral( entity ) ) + " ---", sf::Color::Yellow );

    if ( auto *cmp = reg().try_get<Cmp::AnimData>( entity ) )
    {
      auto sprite_idx = std::to_string( cmp->getFrameIndexOffset() );
      text_line( " " + cmp->m_sprite_type.str() + " [" + sprite_idx + "]" );
    }
    if ( auto reserved_sm = m_reserved_sm.lock(); reserved_sm and not reserved_sm->at( pos_cmp ).empty() )
    {
      text_line( "  Reserved", sf::Color::Red );
    }
    if ( reg().all_of<Cmp::Npc::NoPathFinding>( entity ) ) text_line( "  NpcNoPathFinding", sf::Color::Red );
    if ( reg().all_of<Cmp::Player::NoPath>( entity ) ) text_line( " NoPath", sf::Color::Red );
    if ( reg().all_of<Cmp::Player::PendingNoPath>( entity ) ) text_line( " PendingNoPath", sf::Color::Yellow );
    if ( reg().all_of<Cmp::Moveable>( entity ) ) text_line( " Moveable", sf::Color::Green );
    if ( reg().all_of<Cmp::SelectedPosition>( entity ) ) text_line( " Selected", sf::Color::Green );
    if ( reg().all_of<Cmp::Exit>( entity ) ) text_line( " Exit", sf::Color::Green );
    if ( reg().all_of<Cmp::Obstacle>( entity ) ) text_line( " Obstacle", sf::Color::Green );
    if ( reg().all_of<Cmp::Crypt::PassageBlock>( entity ) ) text_line( " PassageBlock", sf::Color::Cyan );
    if ( reg().all_of<Cmp::FootStepTimer>( entity ) ) text_line( " Footsteps", sf::Color::Cyan );
    if ( reg().all_of<Cmp::Ruin::Cobweb>( entity ) ) text_line( " Cobweb", sf::Color::Cyan );
    if ( reg().all_of<Cmp::Wall>( entity ) ) text_line( " Wall", sf::Color::Cyan );
    if ( reg().all_of<Cmp::Crypt::PassageDoor>( entity ) ) text_line( " PassageDoor", sf::Color::Green );
    if ( reg().all_of<Cmp::Crypt::RoomLavaPitCell>( entity ) ) text_line( " RoomLavaPitCell", sf::Color{ 255, 165, 0 } );
    if ( reg().all_of<Cmp::Crypt::RoomLavaPit>( entity ) ) text_line( " RoomLavaPit", sf::Color{ 255, 165, 0 } );
    if ( reg().all_of<Cmp::Armable>( entity ) ) text_line( " Armable", sf::Color::Green );
    if ( reg().all_of<Cmp::Armed>( entity ) ) text_line( " Armed", sf::Color::Red );
    if ( reg().all_of<Cmp::Grave::ExitSegment>( entity ) ) text_line( " ExitSegment", sf::Color::Cyan );
    if ( reg().all_of<Cmp::Grave::Segment>( entity ) ) text_line( " Segment", sf::Color::Cyan );
    if ( reg().all_of<Cmp::Grave::MultiBlock>( entity ) ) text_line( " MultiBlock", sf::Color::Cyan );
    if ( reg().all_of<Cmp::Altar::Segment>( entity ) ) text_line( " Segment", sf::Color::Cyan );
    if ( reg().all_of<Cmp::Altar::MultiBlock>( entity ) ) text_line( " MultiBlock", sf::Color::Cyan );
    if ( reg().all_of<Cmp::PlantSegment>( entity ) ) text_line( " PlantSegment", sf::Color::Cyan );
    if ( reg().all_of<Cmp::PlantMultiBlock>( entity ) ) text_line( " PlantMultiBlock", sf::Color::Cyan );
    if ( reg().all_of<Cmp::Crypt::BuildingSegment>( entity ) ) text_line( " CryptSegment", sf::Color::Cyan );
    if ( reg().all_of<Cmp::Crypt::BuildingMultiBlock>( entity ) ) text_line( " BuildingMultiBlock", sf::Color::Cyan );
    if ( reg().all_of<Cmp::Ruin::BuildingSegment>( entity ) ) text_line( " RuinSegment", sf::Color::Cyan );
    if ( reg().all_of<Cmp::Ruin::BuildingMultiBlock>( entity ) ) text_line( " BuildingMultiBlock", sf::Color::Cyan );
    if ( reg().all_of<Cmp::HealingSpringBuildingSegment>( entity ) ) text_line( " HealingSpringSegment", sf::Color::Cyan );
    if ( reg().all_of<Cmp::HealingSpringBuildingMultiBlock>( entity ) ) text_line( " HealingSpringBuildingMultiBlock", sf::Color::Cyan );
    if ( reg().all_of<Cmp::Crypt::Entrance>( entity ) ) text_line( " Entrance", sf::Color::Cyan );
    if ( reg().all_of<Cmp::Ruin::Entrance>( entity ) ) text_line( " Entrance", sf::Color::Cyan );
    if ( reg().all_of<Cmp::HealingSpringEntrance>( entity ) ) text_line( " HealingSpringEntrance", sf::Color::Cyan );
    if ( reg().all_of<Cmp::WorldItem>( entity ) ) text_line( " WorldItem", sf::Color::Green );
    if ( reg().all_of<Cmp::Player::Illuminated>( entity ) ) text_line( " Illuminated", sf::Color::Yellow );
    if ( reg().all_of<Cmp::Grave::TrailMarker>( entity ) ) text_line( " TrailMarker", sf::Color::Green );
    if ( reg().all_of<Cmp::Grave::TrailStart>( entity ) ) text_line( " TrailStart", sf::Color::Yellow );
    if ( auto *cmp = reg().try_get<Cmp::Npc::SpawnedBy>( entity ) ) text_line( " SpawnedBy: " + cmp->m_source.str(), sf::Color::White );

    if ( auto *cmp = reg().try_get<Cmp::UUID>( entity ) ) text_line( " " + cmp->str(), sf::Color::White );

    ImGui::Text( "  Size: [ %d , %d ]   Pos: [ %d , %d ]", static_cast<int>( pos_cmp.size.x ), static_cast<int>( pos_cmp.size.y ),
                 static_cast<int>( pos_cmp.position.x ), static_cast<int>( pos_cmp.position.y ) );

    if ( auto *cmp = reg().try_get<Cmp::ZOrderValue>( entity ) ) { ImGui::Text( "  ZOrder: %s", std::to_string( cmp->get() ).c_str() ); }
  }
  ImGui::End();
}

void DebugOverlaySystem::render_lerp_positions()
{
  auto lerp_view = reg().view<Cmp::LerpPosition, Cmp::Direction, Cmp::Npc::NPC, Cmp::Position>();
  for ( auto [entity, lerp_pos_cmp, dir_cmp, npc_cmp, npc_pos_cmp] : lerp_view.each() )
  {
    sf::RectangleShape lerp_start_pos_rect( Constants::kGridSizePxF );
    lerp_start_pos_rect.setPosition( lerp_pos_cmp.m_start );
    lerp_start_pos_rect.setFillColor( sf::Color::Transparent );
    lerp_start_pos_rect.setOutlineColor( sf::Color::Yellow );
    lerp_start_pos_rect.setOutlineThickness( 1.f );
    draw_world( lerp_start_pos_rect );

    sf::RectangleShape lerp_stop_pos_rect( Constants::kGridSizePxF );
    lerp_stop_pos_rect.setPosition( lerp_pos_cmp.m_target );
    lerp_stop_pos_rect.setFillColor( sf::Color::Transparent );
    lerp_stop_pos_rect.setOutlineColor( sf::Color::Cyan );
    lerp_stop_pos_rect.setOutlineThickness( 1.f );
    draw_world( lerp_stop_pos_rect );
  }
}

void DebugOverlaySystem::render_square( sf::Vector2f pos, sf::Vector2f size, sf::Color color )
{
  sf::RectangleShape rect( size );
  rect.setPosition( pos );
  rect.setFillColor( sf::Color::Transparent );
  rect.setOutlineColor( color );
  rect.setOutlineThickness( 1.f );
  draw_world( rect );
}

void DebugOverlaySystem::render_spatial_grid_neighbours( const Cmp::Position &query_pos, sf::Color color, PathFinding::QueryCompass query_compass )
{
  if ( PathFinding::SpatialHashGridSharedPtr spatialgrid_ptr = m_npc_navmesh.lock() )
  {
    std::vector<entt::entity> neighbours_list = spatialgrid_ptr->neighbours( Cmp::Position( query_pos.position, query_pos.size ), query_compass );
    for ( auto neighbour_entt : neighbours_list )
    {
      auto *neighbour_pos = reg().try_get<Cmp::Position>( neighbour_entt );
      if ( not neighbour_pos ) continue;
      if ( reg().any_of<Cmp::Player::Character, Cmp::Npc::NPC>( neighbour_entt ) ) continue;

      sf::RectangleShape rectangle;
      rectangle.setSize( neighbour_pos->size );
      rectangle.setPosition( neighbour_pos->position );
      rectangle.setFillColor( sf::Color::Transparent );
      rectangle.setOutlineThickness( 1.f );
      rectangle.setOutlineColor( color );
      draw_world( rectangle );
    }
  }
}

void DebugOverlaySystem::render_pathfinding_vector( const Cmp::Position &start_pos_cmp, const Cmp::Position &end_pos_cmp, sf::Color color,
                                                   PathFinding::QueryCompass query_compass, std::span<const sf::Vector2i> footprint_cells )
{
  if ( not Utils::is_visible_in_view( RenderSystem::get_world_view(), start_pos_cmp ) ) return;

  if ( PathFinding::SpatialHashGridSharedPtr spatialgrid_ptr = m_npc_navmesh.lock() )
  {
    // Mirror Utils::Npc::pathfind_toward: A* needs a grid-aligned goal, but the player (and an NPC mid-lerp) sit off-grid.
    const Cmp::Position grid_start( Utils::snap_to_grid( start_pos_cmp.position, Utils::Rounding::TOWARDS_ZERO ), start_pos_cmp.size );
    const Cmp::Position grid_goal( Utils::snap_to_grid( end_pos_cmp.position, Utils::Rounding::TOWARDS_ZERO ), end_pos_cmp.size );
    std::vector<PathFinding::PathNode> path = PathFinding::astar( reg(), *spatialgrid_ptr, grid_start, grid_goal, query_compass, {},
                                                                  footprint_cells );

    for ( auto pathnode : path )
    {
      auto expand_lever_pos_hitbox = Cmp::RectBounds::scaled( pathnode.pos.position, pathnode.pos.size, 0.2f );
      sf::RectangleShape rectangle;
      rectangle.setSize( expand_lever_pos_hitbox.size() );
      rectangle.setPosition( expand_lever_pos_hitbox.position() );
      rectangle.setFillColor( sf::Color::Transparent );
      rectangle.setOutlineColor( color );
      rectangle.setOutlineThickness( 1.f );
      draw_world( rectangle );
    }
  }
}

} // namespace Game::Sys
