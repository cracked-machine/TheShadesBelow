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
#include <Components/Grave/ExitSegment.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Grave/Segment.hpp>
#include <Components/Hazard/CorruptionCell.hpp>
#include <Components/Hazard/SinkholeCell.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LastDirection.hpp>
#include <Components/LerpPosition.hpp>
#include <Components/Moveable.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
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
#include <Systems/Render/RenderDebugSystem.hpp>
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
#include <set>
#include <string>
#include <tracy/Tracy.hpp>

namespace Game::Sys
{

RenderDebugSystem::RenderDebugSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : RenderSystem( reg, window, sound_bank ),
      m_dbg_ui_data( std::make_unique<Render::UiData>( "res/ui/dbg.json" ) ),
      m_debug_overlay_tex( { 1, 1 } )
{
  SPDLOG_DEBUG( "RenderDebugSystem initialized" );
}

void RenderDebugSystem::render_debug()
{
  if ( Utils::scene_setting<Cmp::SceneSettings::ShowNavmesh>( reg() ).enabled ) { PROFILED( render_navmesh() ); }
  if ( Utils::scene_setting<Cmp::SceneSettings::ShowPathFinding>( reg() ).enabled ) { PROFILED( render_pathfinding() ); }
  if ( Utils::scene_setting<Cmp::SceneSettings::ShowDebugStats>( reg() ).enabled ) { PROFILED( render_debug_stats() ); }
}

void RenderDebugSystem::render_navmesh()
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

void RenderDebugSystem::render_pathfinding()
{
  const Cmp::Position player_pos_cmp = Utils::Player::get_position( reg() );
  Cmp::Position player_center_hitbox( player_pos_cmp.getCenter(), { 1.f, 1.f } );
  render_square( player_center_hitbox.position, player_center_hitbox.size, sf::Color::Blue );
  render_lerp_positions();
  render_spatial_grid_neighbours( player_center_hitbox, sf::Color::Cyan, PathFinding::QueryCompass::CARDINAL );

  for ( auto [npc_entt, npc_cmp, npc_pos_cmp, anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::AnimData>().each() )
  {
    auto query_compass = PathFinding::QueryCompass::CARDINAL;
    if ( anim_cmp.m_sprite_type.contains( "sprite.ghost" ) ) query_compass = PathFinding::QueryCompass::BOTH;
    Cmp::Position npc_center_hitbox( npc_pos_cmp.getCenter(), { 1.f, 1.f } );

    render_spatial_grid_neighbours( npc_center_hitbox, sf::Color::Magenta, query_compass );
    render_pathfinding_vector( npc_pos_cmp, player_pos_cmp, sf::Color::White, query_compass );
  }
}

void RenderDebugSystem::render_debug_stats()
{
  // these debug shapes are only drawn within the current view to prevent FPS drops
  ZoneScopedN( "RenderDebugUI" );

  render_square_for_floatrect_cmp<Cmp::Crypt::RoomLavaPitCell>( sf::Color( 254, 128, 32 ), 0.5f );
  render_square_for_floatrect_cmp<Cmp::Crypt::RoomOpen>( sf::Color::Green, 1.f );
  render_square_for_floatrect_cmp<Cmp::Crypt::RoomStart>( sf::Color::Blue, 1.f );
  render_square_for_floatrect_cmp<Cmp::Crypt::RoomEnd>( sf::Color::Yellow, 1.f );
  render_square_for_floatrect_cmp<Cmp::Crypt::RoomClosed>( sf::Color::Red, 1.f );
  render_square_for_vector2f_cmp<Cmp::Crypt::PassageBlock>( sf::Color::Black, 1.f );

  PROFILED( begin_debug_overlay( m_window.getSize() ) );
  PROFILED( render_ui_misc_stats() );
  PROFILED( render_ui_zorder_list() );
  PROFILED( render_ui_npc_list() );
  PROFILED( render_ui_entity_inspect() );
  for ( auto [selected_entt, selected_cmp, pos_cmp] : reg().view<Cmp::SelectedPosition, Cmp::Position>().each() )
  {
    if ( not Utils::is_visible_in_view( get_screen_view(), pos_cmp ) ) continue;
    PROFILED( render_square( pos_cmp.position, pos_cmp.size, sf::Color::Yellow ) );
  }

  PROFILED( end_debug_overlay() );
  for ( auto [ps_owner_entt, ps_owner_cmp] : reg().view<Cmp::Particle::SpriteOwner>().each() )
  {
    auto emitter_pos = ps_owner_cmp.sprite->get_emitter_position();
    auto dot = sf::CircleShape( 1 );
    dot.setPosition( emitter_pos );
    dot.setFillColor( sf::Color::Cyan );
    dot.setOutlineColor( sf::Color::Cyan );
    draw_world( dot );
  }

  auto half_view = Cmp::RectBounds::scaled( Utils::calculate_view_bounds( Sys::RenderSystem::get_world_view() ), 0.5f );
  render_square( half_view.getBounds().position, half_view.getBounds().size, sf::Color::Red );

  draw_debug_overlay();
}

void RenderDebugSystem::render_ui_misc_stats()
{
  if ( not m_dbg_ui_data ) { return; }

  for ( const auto &ui_label : m_dbg_ui_data->m_labels )
  {
    if ( ui_label.name != "entity_stats" ) { continue; }

    TextColumn draw_line{ .self = *this, .cache_key = "entity_stats", .origin = ui_label.rect.position, .font_size = 18, .line_height = 22.f };

    draw_line( "--- Stats ---", sf::Color::Yellow );

    draw_line( " Player position - [" + std::to_string( static_cast<int>( Utils::Player::get_position( reg() ).x() ) ) + " , " +
               std::to_string( static_cast<int>( Utils::Player::get_position( reg() ).y() ) ) + "]" );

    sf::Vector2i mouse_pixel_pos = sf::Mouse::getPosition( m_window );
    sf::Vector2f mouse_world_pos = m_window.mapPixelToCoords( mouse_pixel_pos, RenderSystem::get_world_view() );
    draw_line( " Mouse position - [" + std::to_string( static_cast<int>( mouse_world_pos.x ) ) + " , " +
               std::to_string( static_cast<int>( mouse_world_pos.y ) ) + "]" );

    draw_line( " Player current direction - [" + std::to_string( Utils::Player::get_direction( reg() ).x ) + " , " +
               std::to_string( Utils::Player::get_direction( reg() ).y ) + "]" );
    draw_line( " Player last direction - [" + std::to_string( Utils::Player::get_last_direction( reg() ).x ) + " , " +
               std::to_string( Utils::Player::get_last_direction( reg() ).y ) + "]" );

    draw_line( " Entities - " + std::to_string( reg().view<entt::entity>().size() ) );
    draw_line( " NPCs - " + std::to_string( reg().view<Cmp::Npc::NPC>().size() ) );
    draw_line( " Corruption - " + std::to_string( reg().view<Cmp::CorruptionCell>().size() ) );
    draw_line( " Sinkhole - " + std::to_string( reg().view<Cmp::SinkholeCell>().size() ) );
    draw_line( " CryptPassageBlocks - " + std::to_string( reg().view<Cmp::Crypt::PassageBlock>().size() ) );
  }
}

void RenderDebugSystem::render_ui_zorder_list()
{

  if ( not m_dbg_ui_data )
  {
    SPDLOG_CRITICAL( "UiData object is not initialised. Cannot draw zorder list overlay" );
    return;
  }

  // clang-format off
  std::set<Sprites::SpriteMetaType> exclusions = {
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

  for ( const auto &ui_label : m_dbg_ui_data->m_labels )
  {
    if ( ui_label.name != "zorder_list" ) { continue; }

    TextColumn draw_line{ .self = *this, .cache_key = "zorder_list", .origin = ui_label.rect.position, .font_size = 18, .line_height = 22.f };

    draw_line( "--- Z Order List ---", sf::Color::Yellow );

    // float count = 0;
    for ( const auto &zorder_entry : s_zorder_queue )
    {

      std::string name;
      auto *sprite_anim_cmp = reg().try_get<Cmp::AnimData>( zorder_entry.e );
      if ( sprite_anim_cmp )
      {
        if ( exclusions.find( sprite_anim_cmp->m_sprite_type ) != exclusions.end() ) { continue; }
        name = sprite_anim_cmp->m_sprite_type;
      }

      auto *particle_sprite_owner = reg().try_get<Cmp::Particle::SpriteOwner>( zorder_entry.e );
      if ( particle_sprite_owner )
      {
        if ( exclusions.find( particle_sprite_owner->sprite->get_tag() ) != exclusions.end() ) { continue; }
        name = particle_sprite_owner->sprite->get_tag();
      }

      if ( name.empty() ) continue;
      // clang-format off
      draw_line(" Z:" + std::to_string(static_cast<int>(zorder_entry.z)) + 
                " E:" + std::to_string(static_cast<uint32_t>( zorder_entry.e )) +
                " - " + name);
      // clang-format on
    }
  }
}

void RenderDebugSystem::render_ui_npc_list()
{
  if ( not m_dbg_ui_data )
  {
    SPDLOG_CRITICAL( "UiData object is not initialised. Cannot draw zorder list overlay" );
    return;
  }

  for ( const auto &ui_label : m_dbg_ui_data->m_labels )
  {
    if ( ui_label.name != "npc_list" ) { continue; }

    TextColumn draw_line{ .self = *this, .cache_key = "npc_list", .origin = ui_label.rect.position, .font_size = 18, .line_height = 22.f };

    draw_line( "--- NPCs ---", sf::Color::Yellow );

    auto npc_view = reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::AnimData>();
    for ( auto [npc_entity, npc_cmp, npc_pos_cmp, npc_anim_cmp] : npc_view.each() )
    {
      // clang-format off
      draw_line( " " + std::to_string( entt::to_integral( npc_entity ) ) +
                 ": [" + std::to_string( static_cast<int>( npc_pos_cmp.position.x ) ) +
                 "," + std::to_string( static_cast<int>( npc_pos_cmp.position.y ) ) + "] - " +
                 npc_anim_cmp.m_sprite_type);
      // clang-format on
    }
  }
}

void RenderDebugSystem::render_ui_entity_inspect()
{
  if ( not m_dbg_ui_data ) { return; }

  sf::Vector2i mouse_pixel_pos = sf::Mouse::getPosition( m_window );
  sf::Vector2f mouse_world_pos = m_window.mapPixelToCoords( mouse_pixel_pos, RenderSystem::get_world_view() );

  for ( const auto &ui_label : m_dbg_ui_data->m_labels )
  {
    if ( ui_label.name != "inspect_list" ) { continue; }

    TextColumn draw_line{ .self = *this, .cache_key = "inspect_list", .origin = ui_label.rect.position, .font_size = 18, .line_height = 22.f };

    auto position_view = reg().view<Cmp::Position>();
    for ( auto [entity, pos_cmp] : position_view.each() )
    {
      if ( not Utils::is_visible_in_view( Sys::RenderSystem::get_world_view(), pos_cmp ) ) continue;
      if ( not pos_cmp.contains( mouse_world_pos ) ) { continue; }

      // Header: entity ID
      draw_line( "--- Entity #" + std::to_string( entt::to_integral( entity ) ) + " ---", sf::Color::Yellow );

      if ( auto *cmp = reg().try_get<Cmp::AnimData>( entity ) )
      {
        auto sprite_idx = std::to_string( cmp->getFrameIndexOffset() );
        draw_line( " " + cmp->m_sprite_type + " [" + sprite_idx + "]" );
      }
      if ( auto reserved_sm = m_reserved_sm.lock(); reserved_sm && not reserved_sm->at( pos_cmp ).empty() )
      {
        draw_line( "  Reserved", sf::Color::Red );
      }
      if ( reg().all_of<Cmp::Npc::NoPathFinding>( entity ) ) draw_line( "  NpcNoPathFinding", sf::Color::Red );
      if ( reg().all_of<Cmp::Player::NoPath>( entity ) ) draw_line( " NoPath", sf::Color::Red );
      if ( reg().all_of<Cmp::Player::PendingNoPath>( entity ) ) draw_line( " PendingNoPath", sf::Color::Yellow );
      if ( reg().all_of<Cmp::Moveable>( entity ) ) draw_line( " Moveable", sf::Color::Green );
      if ( reg().all_of<Cmp::SelectedPosition>( entity ) ) draw_line( " Selected", sf::Color::Green );
      if ( reg().all_of<Cmp::Exit>( entity ) ) draw_line( " Exit", sf::Color::Green );
      if ( reg().all_of<Cmp::Obstacle>( entity ) ) draw_line( " Obstacle", sf::Color::Green );
      if ( reg().all_of<Cmp::Crypt::PassageBlock>( entity ) ) draw_line( " PassageBlock", sf::Color::Cyan );
      if ( reg().all_of<Cmp::FootStepTimer>( entity ) ) draw_line( " Footsteps", sf::Color::Cyan );
      if ( reg().all_of<Cmp::Ruin::Cobweb>( entity ) ) draw_line( " Cobweb", sf::Color::Cyan );
      if ( reg().all_of<Cmp::Wall>( entity ) ) draw_line( " Wall", sf::Color::Cyan );
      if ( reg().all_of<Cmp::Crypt::PassageDoor>( entity ) ) draw_line( " PassageDoor", sf::Color::Green );
      if ( reg().all_of<Cmp::Crypt::RoomLavaPitCell>( entity ) ) draw_line( " RoomLavaPitCell", sf::Color{ 255, 165, 0 } );
      if ( reg().all_of<Cmp::Crypt::RoomLavaPit>( entity ) ) draw_line( " RoomLavaPit", sf::Color{ 255, 165, 0 } );
      if ( reg().all_of<Cmp::Armable>( entity ) ) draw_line( " Armable", sf::Color::Green );
      if ( reg().all_of<Cmp::Armed>( entity ) ) draw_line( " Armed", sf::Color::Red );
      if ( reg().all_of<Cmp::Grave::ExitSegment>( entity ) ) draw_line( " ExitSegment", sf::Color::Cyan );
      if ( reg().all_of<Cmp::Grave::Segment>( entity ) ) draw_line( " Segment", sf::Color::Cyan );
      if ( reg().all_of<Cmp::Grave::MultiBlock>( entity ) ) draw_line( " MultiBlock", sf::Color::Cyan );
      if ( reg().all_of<Cmp::Altar::Segment>( entity ) ) draw_line( " Segment", sf::Color::Cyan );
      if ( reg().all_of<Cmp::Altar::MultiBlock>( entity ) ) draw_line( " MultiBlock", sf::Color::Cyan );
      if ( reg().all_of<Cmp::PlantSegment>( entity ) ) draw_line( " PlantSegment", sf::Color::Cyan );
      if ( reg().all_of<Cmp::PlantMultiBlock>( entity ) ) draw_line( " PlantMultiBlock", sf::Color::Cyan );
      if ( reg().all_of<Cmp::Crypt::BuildingSegment>( entity ) ) draw_line( " CryptSegment", sf::Color::Cyan );
      if ( reg().all_of<Cmp::Crypt::BuildingMultiBlock>( entity ) ) draw_line( " BuildingMultiBlock", sf::Color::Cyan );
      if ( reg().all_of<Cmp::Ruin::BuildingSegment>( entity ) ) draw_line( " RuinSegment", sf::Color::Cyan );
      if ( reg().all_of<Cmp::Ruin::BuildingMultiBlock>( entity ) ) draw_line( " BuildingMultiBlock", sf::Color::Cyan );
      if ( reg().all_of<Cmp::HealingSpringBuildingSegment>( entity ) ) draw_line( " HealingSpringSegment", sf::Color::Cyan );
      if ( reg().all_of<Cmp::HealingSpringBuildingMultiBlock>( entity ) ) draw_line( " HealingSpringBuildingMultiBlock", sf::Color::Cyan );
      if ( reg().all_of<Cmp::Crypt::Entrance>( entity ) ) draw_line( " Entrance", sf::Color::Cyan );
      if ( reg().all_of<Cmp::Ruin::Entrance>( entity ) ) draw_line( " Entrance", sf::Color::Cyan );
      if ( reg().all_of<Cmp::HealingSpringEntrance>( entity ) ) draw_line( " HealingSpringEntrance", sf::Color::Cyan );
      if ( reg().all_of<Cmp::WorldItem>( entity ) ) draw_line( " WorldItem", sf::Color::Green );
      if ( reg().all_of<Cmp::Player::Illuminated>( entity ) ) draw_line( " Illuminated", sf::Color::Yellow );

      if ( auto *cmp = reg().try_get<Cmp::UUID>( entity ) ) draw_line( " " + cmp->str(), sf::Color::White );

      auto posx = std::to_string( static_cast<int>( pos_cmp.position.x ) );
      auto posy = std::to_string( static_cast<int>( pos_cmp.position.y ) );
      auto sizex = std::to_string( static_cast<int>( pos_cmp.size.x ) );
      auto sizey = std::to_string( static_cast<int>( pos_cmp.size.y ) );
      draw_line( std::string( "  Size: [ " )
                     .append( sizex )
                     .append( " , " )
                     .append( sizey )
                     .append( " ]   Pos: [ " )
                     .append( posx )
                     .append( " , " )
                     .append( posy )
                     .append( " ]" ) );

      if ( auto *cmp = reg().try_get<Cmp::ZOrderValue>( entity ) )
      {
        auto zorder = std::to_string( cmp->getZOrder() );
        draw_line( "  ZOrder: " + zorder );
      }
    }
  }
}

void RenderDebugSystem::render_lerp_positions()
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

void RenderDebugSystem::render_square( sf::Vector2f pos, sf::Vector2f size, sf::Color color )
{
  sf::RectangleShape rect( size );
  rect.setPosition( pos );
  rect.setFillColor( sf::Color::Transparent );
  rect.setOutlineColor( color );
  rect.setOutlineThickness( 1.f );
  draw_world( rect );
}

void RenderDebugSystem::render_spatial_grid_neighbours( const Cmp::Position &query_pos, sf::Color color, PathFinding::QueryCompass query_compass )
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

void RenderDebugSystem::render_pathfinding_vector( const Cmp::Position &start_pos_cmp, const Cmp::Position &end_pos_cmp, sf::Color color,
                                                     PathFinding::QueryCompass query_compass )
{
  if ( not Utils::is_visible_in_view( RenderSystem::get_world_view(), start_pos_cmp ) ) return;

  if ( PathFinding::SpatialHashGridSharedPtr spatialgrid_ptr = m_npc_navmesh.lock() )
  {
    // Mirror Utils::Npc::pathfind_toward: A* needs a grid-aligned goal, but the player (and an NPC mid-lerp) sit off-grid.
    const Cmp::Position grid_start( Utils::snap_to_grid( start_pos_cmp.position, Utils::Rounding::TOWARDS_ZERO ), start_pos_cmp.size );
    const Cmp::Position grid_goal( Utils::snap_to_grid( end_pos_cmp.position, Utils::Rounding::TOWARDS_ZERO ), end_pos_cmp.size );
    std::vector<PathFinding::PathNode> path = PathFinding::astar( reg(), *spatialgrid_ptr, grid_start, grid_goal, query_compass );

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
