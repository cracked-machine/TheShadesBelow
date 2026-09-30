
#include <Components/AbsoluteAlpha.hpp>
#include <Components/AbsoluteOffset.hpp>
#include <Components/AbsoluteRenderOffset.hpp>
#include <Components/AbsoluteRotation.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armed.hpp>
#include <Components/Crypt/BuildingMultiBlock.hpp>
#include <Components/Crypt/Chest.hpp>
#include <Components/Crypt/Entrance.hpp>
#include <Components/Crypt/InteriorMultiBlock.hpp>
#include <Components/Crypt/Lever.hpp>
#include <Components/Crypt/PassageBlock.hpp>
#include <Components/Crypt/RoomClosed.hpp>
#include <Components/Crypt/RoomEnd.hpp>
#include <Components/Crypt/RoomLavaPit.hpp>
#include <Components/Crypt/RoomLavaPitCell.hpp>
#include <Components/Crypt/RoomOpen.hpp>
#include <Components/Crypt/RoomStart.hpp>
#include <Components/Exit.hpp>
#include <Components/FractalCurve.hpp>
#include <Components/Grave/ExitMultiBlock.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/ScryingBall.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/LastDirection.hpp>
#include <Components/Moveable.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/Shockwave.hpp>
#include <Components/ObstacleCap.hpp>
#include <Components/Persistent/ArmedBlinkFreq.hpp>
#include <Components/Persistent/CameraSmoothSpeed.hpp>
#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/ArrowCompass.hpp>
#include <Components/Player/BlastRadius.hpp>
#include <Components/Player/CadaverCount.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/Curse.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Ruin/BuildingMultiBlock.hpp>
#include <Components/SceneSettings/Shaders.hpp>
#include <Components/SceneSettings/ShowDebugStats.hpp>
#include <Components/SceneSettings/ShowNavmesh.hpp>
#include <Components/SceneSettings/ShowPathFinding.hpp>
#include <Components/SelectedPosition.hpp>
#include <Components/Spring/HealingSpringBuildingMultiBlock.hpp>
#include <Components/Wall.hpp>
#include <Components/Weapons/Arrow.hpp>
#include <Components/Wormhole/MultiBlock.hpp>
#include <Components/ZOrderValue.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <Shaders/BaseShaderSprite.hpp>
#include <Shaders/DarkModeShader.hpp>
#include <Shaders/DrippingBloodShader.hpp>
#include <Shaders/FloodWaterShader.hpp>
#include <Shaders/MistShader.hpp>
#include <Shaders/NightStaticShader.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Sprites/VertexFloor.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/Render/RenderOverlaySystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/ShaderSystem.hpp>
#include <Systems/Threats/HazardFieldSystemImpl.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Profiling.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <memory>
#include <optional>
#include <ranges>
#include <tracy/Tracy.hpp>

namespace Game::Sys
{

RenderGameSystem::RenderGameSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory,
                                    Audio::SoundBank &sound_bank )
    : RenderSystem( reg, window, sprite_factory, sound_bank )
{
  SPDLOG_DEBUG( "RenderGameSystem initialized" );
}

RenderGameSystem::~RenderGameSystem() = default;

void RenderGameSystem::render_game( sf::Time dt, RenderOverlaySystem &render_overlay_sys,
                                    const PathFinding::SpatialHashGridSharedPtr &render_position_grid )
{
  using namespace Sprites;

  const Cmp::Position player_pos_cmp = Utils::Player::get_position( reg() );

  // make sure the local view is centered on the player mid-point and not at their top-left corner
  // (otherwise this makes views, shaders, etc look off-center)
  PROFILED( update_camera( dt ) );

  // re-populate the z-order queue with the latest entity/component data
  PROFILED( refresh_z_order_queue( render_position_grid ) );

  const bool show_debug_stats = Utils::scene_setting<Cmp::SceneSettings::ShowDebugStats>( reg() ).enabled;

  // render the zorder queue, anything after this is treated as an "overlay" to the main render pipeline
  PROFILED( render_zorder_queue( render_overlay_sys ) );

  PROFILED( render_shockwaves() );
  PROFILED( render_arrow_compass() );

  PROFILED( render_lightning_strike() );
  PROFILED( render_obstacle_cracks() );

  PROFILED( render_overlay_sys.render_shop_inventory_overlay() );
  PROFILED( render_overlay_sys.render_grimoire_inventory_overlay() );

  // lava pit outline
  render_overlay_sys.render_square_for_floatrect_cmp<Cmp::Crypt::RoomLavaPit>( sf::Color( 16, 16, 16 ), 0.5f );

  if ( Utils::scene_setting<Cmp::SceneSettings::ShowNavmesh>( reg() ).enabled ) { render_overlay_sys.render_navmesh(); }
  if ( Utils::scene_setting<Cmp::SceneSettings::ShowPathFinding>( reg() ).enabled )
  {

    Cmp::Position player_center_hitbox( player_pos_cmp.getCenter(), { 1.f, 1.f } );
    render_overlay_sys.render_square( player_center_hitbox.position, player_center_hitbox.size, sf::Color::Blue );
    render_overlay_sys.render_lerp_positions();
    render_overlay_sys.render_spatial_grid_neighbours( player_center_hitbox, sf::Color::Cyan, PathFinding::QueryCompass::CARDINAL );

    for ( auto [npc_entt, npc_cmp, npc_pos_cmp, anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::AnimData>().each() )
    {
      auto query_compass = PathFinding::QueryCompass::CARDINAL;
      if ( anim_cmp.m_sprite_type.contains( "sprite.ghost" ) ) query_compass = PathFinding::QueryCompass::BOTH;
      Cmp::Position npc_center_hitbox( npc_pos_cmp.getCenter(), { 1.f, 1.f } );

      render_overlay_sys.render_spatial_grid_neighbours( npc_center_hitbox, sf::Color::Magenta, query_compass );
      render_overlay_sys.render_pathfinding_vector( npc_pos_cmp, player_pos_cmp, sf::Color::White, query_compass );
    }
  }

  // render normal game UI
  PROFILED( render_overlay_sys.render_ui_outlines() );
  PROFILED( render_overlay_sys.render_ui_icons() );
  PROFILED( render_overlay_sys.render_ui_inventory_icon() );
  PROFILED( render_overlay_sys.render_ui_meters( dt ) );
  PROFILED( render_overlay_sys.render_ui_labels( dt ) );
  PROFILED( render_overlay_sys.render_ui_texts() );
  PROFILED( render_overlay_sys.render_level_depth() );

  auto display_size = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );
  render_overlay_sys.render_crypt_maze_timer( { static_cast<float>( display_size.x ) / 2.f, 0.f }, 100 );

  // these debug shapes are only drawn within the current view to prevent FPS drops
  if ( show_debug_stats )
  {
    ZoneScopedN( "RenderDebugUI" );

    render_overlay_sys.render_square_for_floatrect_cmp<Cmp::Crypt::RoomLavaPitCell>( sf::Color( 254, 128, 32 ), 0.5f );
    render_overlay_sys.render_square_for_floatrect_cmp<Cmp::Crypt::RoomOpen>( sf::Color::Green, 1.f );
    render_overlay_sys.render_square_for_floatrect_cmp<Cmp::Crypt::RoomStart>( sf::Color::Blue, 1.f );
    render_overlay_sys.render_square_for_floatrect_cmp<Cmp::Crypt::RoomEnd>( sf::Color::Yellow, 1.f );
    render_overlay_sys.render_square_for_floatrect_cmp<Cmp::Crypt::RoomClosed>( sf::Color::Red, 1.f );
    render_overlay_sys.render_square_for_vector2f_cmp<Cmp::Crypt::PassageBlock>( sf::Color::Black, 1.f );

    PROFILED( render_overlay_sys.begin_debug_overlay( m_window.getSize() ) );
    PROFILED( render_overlay_sys.render_ui_misc_stats() );
    PROFILED( render_overlay_sys.render_ui_zorder_list( m_zorder_queue_ ) );
    PROFILED( render_overlay_sys.render_ui_npc_list() );
    PROFILED( render_overlay_sys.render_ui_entity_inspect() );
    for ( auto [selected_entt, selected_cmp, pos_cmp] : reg().view<Cmp::SelectedPosition, Cmp::Position>().each() )
    {
      if ( not Utils::is_visible_in_view( get_screen_view(), pos_cmp ) ) continue;
      PROFILED( render_overlay_sys.render_square( pos_cmp.position, pos_cmp.size, sf::Color::Yellow ) );
    }

    PROFILED( render_overlay_sys.end_debug_overlay() );
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
    render_overlay_sys.render_square( half_view.getBounds().position, half_view.getBounds().size, sf::Color::Red );
  }

  if ( show_debug_stats ) render_overlay_sys.draw_debug_overlay( m_window );

  m_window.display();
}

void RenderGameSystem::render_zorder_queue( RenderOverlaySystem &render_overlay_sys )
{
  m_window.clear();

  // render anything with a ZOrderValue component in lowest value first order
  for ( const auto &zorder_entry : m_zorder_queue_ )
  {
    auto entity = zorder_entry.e;
    if ( reg().all_of<Cmp::Position, Cmp::AnimData>( entity ) )
    {
      const auto &pos_cmp = reg().get<Cmp::Position>( entity );
      const auto &anim_cmp = reg().get<Cmp::AnimData>( entity );

      uint8_t alpha_value = 255;
      auto *obst_cmp = reg().try_get<Cmp::AbsoluteAlpha>( entity );
      if ( obst_cmp ) alpha_value = obst_cmp->getAlpha();

      sf::Vector2f new_origin_value = { 0.F, 0.F };
      auto *new_offset_cmp = reg().try_get<Cmp::AbsoluteOffset>( entity );
      if ( new_offset_cmp ) new_origin_value = new_offset_cmp->getOffset();

      sf::Angle new_angle_value = sf::degrees( 0.f );
      auto *new_angle_cmp = reg().try_get<Cmp::AbsoluteRotation>( entity );
      if ( new_angle_cmp ) new_angle_value = sf::degrees( new_angle_cmp->getAngle() );

      sf::FloatRect render_pos_cmp = pos_cmp;
      auto *render_offset_cmp = reg().try_get<Cmp::AbsoluteRenderOffset>( entity );
      if ( render_offset_cmp ) render_pos_cmp.position += render_offset_cmp->getOffset();

      safe_render_sprite_world( anim_cmp.m_sprite_type, render_pos_cmp, anim_cmp.getFrameIndexOffset() + anim_cmp.m_current_frame, { 1.f, 1.f },
                                alpha_value, new_origin_value, new_angle_value );

      if ( reg().any_of<Cmp::SeeingStone>( entity ) )
      {
        const auto &stone_cmp = reg().get<Cmp::SeeingStone>( entity );
        render_seeingstone_doglegs( stone_cmp, pos_cmp );
      }

      if ( reg().any_of<Cmp::Inventory::WearLevel>( entity ) )
      {
        render_overlay_sys.render_wear_level( reg().get<Cmp::Inventory::WearLevel>( entity ).m_level, pos_cmp );
      }

      if ( reg().any_of<Cmp::Armed>( entity ) ) { render_armed_indicator( reg().get<Cmp::Armed>( entity ), pos_cmp ); }
    }
    else if ( reg().all_of<Cmp::Shader::SpriteOwner>( entity ) )
    {
      auto &shader_sprite_owner = reg().get<Cmp::Shader::SpriteOwner>( entity );
      if ( not shader_sprite_owner.sprite ) continue;
      if ( not Utils::scene_setting<Cmp::SceneSettings::Shaders>( reg() ).enabled ) continue;

      if ( shader_sprite_owner.sprite->is_post_process() )
      {
        // Reached this shader's ZOrderValue-driven position in the queue: capture everything drawn so far (including
        // any earlier post-process pass) into its render texture, then composite the shaded result back over the window.
        if ( not shader_sprite_owner.sprite->active() ) continue;
        if ( m_frame_capture.getSize() != m_window.getSize() ) { (void)m_frame_capture.resize( m_window.getSize() ); }
        m_frame_capture.update( m_window );

        auto &render_texture = shader_sprite_owner.sprite->get_render_texture();
        render_texture.clear();
        render_texture.draw( sf::Sprite( m_frame_capture ) );
        render_texture.display();
        draw_screen( *shader_sprite_owner.sprite );
      }
      else { draw_world( *shader_sprite_owner.sprite ); }
    }
    else if ( reg().all_of<Cmp::Particle::SpriteOwner>( entity ) )
    {
      auto &particle_sprite_owner = reg().get<Cmp::Particle::SpriteOwner>( entity );

      if ( particle_sprite_owner.sprite->get_view_type() == Cmp::Particle::ViewType::WORLD )
      {
        // draw in world
        particle_sprite_owner.sprite->set_view_transform( m_window, s_world_view );
        draw_screen( *particle_sprite_owner.sprite );
      }
      else
      {
        // draw in screen (UI) if a candle matches the players inventory
        if ( not particle_sprite_owner.sprite->get_tag().contains( "candle" ) ) continue;
        particle_sprite_owner.sprite->set_view_transform( m_window, m_window.getDefaultView() );
        for ( auto &icon : render_overlay_sys.m_main_ui_data->m_icons )
        {
          if ( icon.name != "inventory_icon" ) continue;
          sf::Vector2f new_emitter_pos = { icon.rect.position.x + ( icon.scale * 8.f ), icon.rect.position.y + ( icon.scale * 6.f ) };
          particle_sprite_owner.sprite->set_emitter_position( new_emitter_pos );
        }
        particle_sprite_owner.sprite->restart();
        draw_screen( *particle_sprite_owner.sprite );
      }
    }
    else if ( reg().all_of<Sprites::Containers::VertexFloor>( entity ) )
    {

      auto &floor_tiles = reg().get<Sprites::Containers::VertexFloor>( entity );
      sf::Vector2f adjusted{ static_cast<float>( floor_tiles.world_grid_offset.x ) * Constants::kGridSizePxF.x,
                             static_cast<float>( floor_tiles.world_grid_offset.y ) * Constants::kGridSizePxF.y };
      floor_tiles.setPosition( adjusted );
      draw_world( floor_tiles );
    }
  }
}

void RenderGameSystem::refresh_z_order_queue( const PathFinding::SpatialHashGridSharedPtr &render_position_grid )
{
  m_render_position_grid = render_position_grid;
  m_zorder_queue_.clear();
  sf::FloatRect view_bounds = Utils::calculate_view_bounds( s_world_view );

  // prevent pop-in/pop-outs when multiblock entities are near the edge of the view
  PROFILED( add_visible_entity_to_z_order_queue<Cmp::Altar::MultiBlock>( m_zorder_queue_, view_bounds ) );
  PROFILED( add_visible_entity_to_z_order_queue<Cmp::Crypt::BuildingMultiBlock>( m_zorder_queue_, view_bounds ) );
  PROFILED( add_visible_entity_to_z_order_queue<Cmp::Grave::MultiBlock>( m_zorder_queue_, view_bounds ) );
  PROFILED( add_visible_entity_to_z_order_queue<Cmp::HealingSpringBuildingMultiBlock>( m_zorder_queue_, view_bounds ) );
  PROFILED( add_visible_entity_to_z_order_queue<Cmp::Crypt::InteriorMultiBlock>( m_zorder_queue_, view_bounds ) );
  PROFILED( add_visible_entity_to_z_order_queue<Cmp::Ruin::BuildingMultiBlock>( m_zorder_queue_, view_bounds ) );
  PROFILED( add_visible_entity_to_z_order_queue<Cmp::Grave::ExitMultiBlock>( m_zorder_queue_, view_bounds ) );
  PROFILED( add_visible_entity_to_z_order_queue<Cmp::Wormhole::MultiBlock>( m_zorder_queue_, view_bounds ) );

  // add any floor tile sets
  PROFILED( add_visible_entity_to_z_order_queue<Sprites::Containers::VertexFloor>( m_zorder_queue_, view_bounds ) );

  // add the wrapper types for all particle and shader sprites so they can be rendered with the other entities
  PROFILED( add_visible_entity_to_z_order_queue<Cmp::Particle::SpriteOwner>( m_zorder_queue_, view_bounds ) );
  PROFILED( add_visible_entity_to_z_order_queue<Cmp::Shader::SpriteOwner>( m_zorder_queue_, view_bounds ) );

  // add other components as normal
  PROFILED( add_visible_entity_to_z_order_queue<Cmp::Position>( m_zorder_queue_, view_bounds ) );

  PROFILED( std::ranges::sort( m_zorder_queue_, []( const ZOrder &a, const ZOrder &b ) { return a.z < b.z; } ) );
}

void RenderGameSystem::init_world_view()
{
  // init world view dimensions
  s_world_view = sf::View( { kWorldViewSizeF.x * 0.5f, kWorldViewSizeF.y * 0.5f }, kWorldViewSizeF );
  s_world_view.setViewport( sf::FloatRect( { 0.f, 0.f }, { 1.f, 1.f } ) );

  auto start_pos = Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( reg() );
  s_world_view.setCenter( start_pos );
}

void RenderGameSystem::update_camera( sf::Time deltaTime )
{

  // Use the player's current position as the target
  auto target_pos = Utils::Player::get_position( reg() );

  // Initialize camera position on first frame to avoid lerping from origin
  if ( !m_camera_initialized )
  {
    m_camera_position = target_pos.position;
    m_camera_initialized = true;
  }

  // Smooth lerp toward target position
  float dt = deltaTime.asSeconds();
  auto camera_smooth_speed = Sys::PersistSystem::get<Cmp::Persist::CameraSmoothSpeed>( reg() ).get_value();
  float t = 1.0f - std::exp( -camera_smooth_speed * dt ); // Exponential smoothing

  m_camera_position.x += ( target_pos.position.x - m_camera_position.x ) * t;
  m_camera_position.y += ( target_pos.position.y - m_camera_position.y ) * t;

  // Snap to target if very close (prevents endless micro-adjustments)
  constexpr float kSnapThreshold = 0.1f;
  if ( std::abs( target_pos.position.x - m_camera_position.x ) < kSnapThreshold &&
       std::abs( target_pos.position.y - m_camera_position.y ) < kSnapThreshold )
  {
    m_camera_position = target_pos.position;
  }

  // Update the view center
  sf::Vector2f view_center = m_camera_position + ( target_pos.size / 2.f );
  s_world_view.setCenter( view_center );
}

void RenderGameSystem::render_shockwaves()
{
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );
  for ( auto [npc_sh_entt, npc_sw_cmp] : reg().view<Cmp::Npc::Shockwave>().each() )
  {
    for ( const auto &segment : npc_sw_cmp.sprite.get_visible_segments() )
    {
      sf::FloatRect segment_bounds = segment.get_bounds( npc_sw_cmp.sprite.get_position(), npc_sw_cmp.sprite.get_radius(),
                                                         npc_sw_cmp.sprite.get_outline_thickness() );

      if ( Utils::is_visible_in_view( view_bounds, segment_bounds ) )
      {
        segment.draw( active_render_target(), sf::RenderStates::Default, npc_sw_cmp.sprite.get_position(), npc_sw_cmp.sprite.get_radius(),
                      npc_sw_cmp.sprite.get_outline_thickness(), npc_sw_cmp.sprite.get_outline_color(), npc_sw_cmp.sprite.get_points_per_segment() );
      }
    }
  }
}

void RenderGameSystem::render_arrow_compass()
{
  static const std::string kNoItem;

  for ( auto [player_entt, pc_cmp, pc_pos_cmp] : reg().view<Cmp::Player::Character, Cmp::Position>().each() )
  {
    auto &compass = reg().get_or_emplace<Cmp::Player::ArrowCompass>( player_entt );

    // single slot inventory; read the item type by reference to avoid a per-frame string copy
    auto inv_view = reg().view<Cmp::PlayerInventorySlot>();
    const auto *inv_slot = inv_view.empty() ? nullptr : &inv_view.get<Cmp::PlayerInventorySlot>( inv_view.front() );
    compass.update_mode( inv_slot ? inv_slot->m_item.item_type : kNoItem );
    if ( compass.mode() == Cmp::Player::ArrowCompass::Mode::NONE ) return;

    const sf::Vector2f player_center = pc_pos_cmp.getCenter();
    compass.refresh_target( reg(), player_center );

    auto placement = compass.placement( player_center, s_world_view );
    if ( not placement ) return;
    safe_render_sprite_world( Cmp::Player::ArrowCompass::kSpriteType, placement->rect, 0, placement->scale, 255, placement->origin,
                              placement->angle );
  }
}

void RenderGameSystem::render_seeingstone_doglegs( const Cmp::SeeingStone &stone_cmp, const Cmp::Position &pos_cmp )
{
  auto draw_dogleg = [this]( sf::Vector2f source_pos, sf::Vector2f target_pos, sf::Color color, float thickness )
  {
    sf::Vector2f corner{};
    if ( target_pos.y - source_pos.y < target_pos.x - source_pos.x ) { corner = sf::Vector2f{ source_pos.x, target_pos.y }; }
    else { corner = sf::Vector2f{ target_pos.x, source_pos.y }; }

    draw_world( Utils::Maths::thick_line_rect( source_pos, corner, color, thickness ) );
    draw_world( Utils::Maths::thick_line_rect( corner, target_pos, color, thickness ) );
  };

  constexpr float kLineThickness = 3.f;
  if ( not stone_cmp.active ) { return; }
  switch ( stone_cmp.target )
  {
    case Cmp::SeeingStone::Target::YELLOW: {
      auto altar_view = reg().view<Cmp::Altar::MultiBlock>();
      for ( auto [altar_entt, altar_cmp] : altar_view.each() )
      {
        // yellow for altar paths
        draw_dogleg( pos_cmp.getCenter(), altar_cmp.getCenter(), sf::Color( 255, 255, 0, 128 ), kLineThickness );
      }
      break;
    }
    case Cmp::SeeingStone::Target::RED: {
      auto crypt_view = reg().view<Cmp::Crypt::Entrance, Cmp::Position>();
      for ( auto [crypt_entt, crypt_cmp, crypt_pos_cmp] : crypt_view.each() )
      {
        // red for crypt paths
        draw_dogleg( pos_cmp.getCenter(), crypt_pos_cmp.getCenter(), sf::Color( 255, 0, 0, 128 ), kLineThickness );
      }
      break;
    }
    case Cmp::SeeingStone::Target::GREEN: {
      auto exit_view = reg().view<Cmp::Exit, Cmp::Position>();
      for ( auto [exit_entt, exit_cmp, exit_pos_cmp] : exit_view.each() )
      {
        draw_dogleg( pos_cmp.getCenter(), exit_pos_cmp.getCenter(), sf::Color( 0, 255, 0, 128 ), kLineThickness );
      }
      break;
    }
    case Cmp::SeeingStone::Target::NONE: {
      break;
    }
  }
}

void RenderGameSystem::render_armed_indicator( const Cmp::Armed &armed_cmp, const Cmp::Position &pos_cmp )
{
  sf::RectangleShape temp_square( Constants::kGridSizePxF );
  temp_square.setPosition( pos_cmp.position );
  temp_square.setOutlineColor( sf::Color::Transparent );
  temp_square.setFillColor( sf::Color::Transparent );
  if ( armed_cmp.getElapsedWarningTime() > armed_cmp.m_warning_delay )
  {
    const float &flash_clk_elapsed_secs = Utils::Player::get_global_bomb_flash_clk( reg() ).getElapsedTime().asSeconds();
    const float &armed_blink_hertz = Sys::PersistSystem::get<Cmp::Persist::ArmedBlinkFreq>( reg() ).get_value();
    const bool flash_on = static_cast<int>( flash_clk_elapsed_secs * armed_blink_hertz * 2.f ) % 2 == 0;
    if ( flash_on )
    {
      temp_square.setOutlineColor( armed_cmp.m_armed_color_border );
      temp_square.setFillColor( armed_cmp.m_armed_color_fill );
    }
  }
  temp_square.setOutlineThickness( 1.f );
  draw_world( temp_square );
}

void RenderGameSystem::render_fractal_curve( const Cmp::FractalCurve &curve )
{
  // Segments are in world-space; convert to screen-space so line thickness is constant regardless of zoom.
  for ( const auto &seg : curve.segments() )
  {
    const auto color = seg.is_main ? curve.m_main_strike_line_color : curve.m_aux_strike_line_color;
    const auto thickness = seg.is_main ? curve.m_main_line_thickness : curve.m_aux_line_thickness;
    draw_screen( Utils::Maths::thick_line_rect( world_to_screen( seg.start ), world_to_screen( seg.end ), color, thickness ) );
  }
}

void RenderGameSystem::render_lightning_strike()
{

  for ( auto [entt, cmp] : reg().view<Cmp::LightningStrike>().each() )
  {
    cmp.timer.start();
    if ( cmp.sequence.size() < 2 ) return;
    render_screen_flash( sf::Color( 255, 255, 255, 180 ) );
    render_fractal_curve( cmp );
    return; // multiple strikes are cued up per event so only render one per frame
  }
}

void RenderGameSystem::render_obstacle_cracks()
{
  for ( auto [ob_crack_entt, ob_crack_cmp] : reg().view<Cmp::ObstacleCrack>().each() )
  {
    if ( ob_crack_cmp.sequence.size() < 2 ) return;
    render_fractal_curve( ob_crack_cmp );
  }
}

void RenderGameSystem::render_screen_flash( sf::Color color )
{

  auto display_res = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );
  auto flash = sf::RectangleShape( sf::Vector2f( display_res ) );
  flash.setPosition( { 0.f, 0.f } );
  flash.setFillColor( color );
  // draw flash in screen view so it covers the whole screen in screen-space
  draw_screen( flash );
}

} // namespace Game::Sys