
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
#include <Components/Crypt/RoomLavaPit.hpp>
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
#include <Components/Spring/HealingSpringBuildingMultiBlock.hpp>
#include <Components/Wall.hpp>
#include <Components/Weapons/Arrow.hpp>
#include <Components/Wormhole/MultiBlock.hpp>
#include <Components/ZOrderValue.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
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
#include <Systems/Render/RenderPassTypes.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/ShaderSystem.hpp>
#include <Systems/Threats/HazardFieldSystemImpl.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Profiling.hpp>
#include <Utils/Utils.hpp>

#include <algorithm>
#include <memory>
#include <optional>
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

void RenderGameSystem::render_game( sf::Time dt, const PathFinding::SpatialHashGridSharedPtr &render_position_grid )
{
  // make sure the local view is centered on the player mid-point and not at their top-left corner
  // (otherwise this makes views, shaders, etc look off-center)
  PROFILED( update_camera( dt ) );

  // re-populate the z-order queue with the latest entity/component data
  PROFILED( s_zorder_queue.refresh( reg(), Utils::calculate_view_bounds( s_world_view ), render_position_grid ) );

  // render the zorder queue, anything after this is treated as an "overlay" to the main render pipeline
  PROFILED( render_zorder_queue() );

  PROFILED( render_shockwaves() );
  PROFILED( render_arrow_compass() );

  PROFILED( render_lightning_strike() );
  PROFILED( render_obstacle_cracks() );

  // lava pit outline
  render_square_for_floatrect_cmp<Cmp::Crypt::RoomLavaPit>( sf::Color( 16, 16, 16 ), 0.5f );
}

void RenderGameSystem::render_zorder_queue()
{
  const bool shaders_enabled = Utils::scene_setting<Cmp::SceneSettings::Shaders>( reg() ).enabled;

  // render anything with a ZOrderValue component in lowest value first order
  for ( const auto &[z, entity] : s_zorder_queue )
  {
    if ( reg().all_of<Cmp::Position, Cmp::AnimData>( entity ) ) { draw_animated_sprite( entity ); }
    else if ( auto *shader_owner = reg().try_get<Cmp::Shader::SpriteOwner>( entity ) )
    {
      if ( shaders_enabled ) draw_shader_sprite( *shader_owner );
    }
    else if ( auto *particle_owner = reg().try_get<Cmp::Particle::SpriteOwner>( entity ) ) { draw_particle_sprite( *particle_owner ); }
    else if ( auto *floor_tiles = reg().try_get<Sprites::Containers::VertexFloor>( entity ) ) { draw_vertex_floor( *floor_tiles ); }
  }
}

void RenderGameSystem::draw_animated_sprite( entt::entity entity )
{
  const auto &[pos_cmp, anim_cmp] = reg().get<Cmp::Position, Cmp::AnimData>( entity );

  const auto *alpha_cmp = reg().try_get<Cmp::AbsoluteAlpha>( entity );
  const auto *offset_cmp = reg().try_get<Cmp::AbsoluteOffset>( entity );
  const auto *rotation_cmp = reg().try_get<Cmp::AbsoluteRotation>( entity );
  const auto *render_offset_cmp = reg().try_get<Cmp::AbsoluteRenderOffset>( entity );

  const uint8_t alpha = alpha_cmp ? alpha_cmp->getAlpha() : 255;
  const sf::Vector2f origin = offset_cmp ? offset_cmp->getOffset() : sf::Vector2f{ 0.f, 0.f };
  const sf::Angle angle = sf::degrees( rotation_cmp ? rotation_cmp->getAngle() : 0.f );
  sf::FloatRect render_rect = pos_cmp;
  if ( render_offset_cmp ) render_rect.position += render_offset_cmp->getOffset();

  safe_render_sprite_world( anim_cmp.m_sprite_type, render_rect, anim_cmp.getFrameIndexOffset() + anim_cmp.m_current_frame, { 1.f, 1.f }, alpha,
                            origin, angle );

  // per-entity decorations drawn on top of the sprite
  if ( const auto *stone_cmp = reg().try_get<Cmp::SeeingStone>( entity ) ) render_seeingstone_doglegs( *stone_cmp, pos_cmp );
  if ( const auto *wear_cmp = reg().try_get<Cmp::Inventory::WearLevel>( entity ) ) render_wear_level( wear_cmp->m_level, pos_cmp );
  if ( const auto *armed_cmp = reg().try_get<Cmp::Armed>( entity ) ) render_armed_indicator( *armed_cmp, pos_cmp );
}

void RenderGameSystem::draw_shader_sprite( Cmp::Shader::SpriteOwner &shader_owner )
{
  if ( not shader_owner.sprite ) return;
  auto &sprite = *shader_owner.sprite;

  if ( not sprite.is_post_process() )
  {
    draw_world( sprite );
    return;
  }

  // Reached this shader's ZOrderValue-driven position in the queue: capture everything drawn so far (including
  // any earlier post-process pass) into its render texture, then composite the shaded result back over the window.
  if ( not sprite.active() ) return;
  if ( m_frame_capture.getSize() != m_window.getSize() ) { (void)m_frame_capture.resize( m_window.getSize() ); }
  m_frame_capture.update( m_window );

  auto &render_texture = sprite.get_render_texture();
  render_texture.clear();
  render_texture.draw( sf::Sprite( m_frame_capture ) );
  render_texture.display();
  draw_screen( sprite );
}

void RenderGameSystem::draw_particle_sprite( Cmp::Particle::SpriteOwner &particle_owner )
{
  // screen-space (UI) particles are drawn by RenderOverlaySystem
  if ( not particle_owner.sprite ) return;
  if ( particle_owner.sprite->get_view_type() != Cmp::Particle::ViewType::WORLD ) return;

  particle_owner.sprite->set_view_transform( m_window, s_world_view );
  draw_screen( *particle_owner.sprite );
}

void RenderGameSystem::draw_vertex_floor( Sprites::Containers::VertexFloor &floor_tiles )
{
  floor_tiles.setPosition( { static_cast<float>( floor_tiles.world_grid_offset.x ) * Constants::kGridSizePxF.x,
                             static_cast<float>( floor_tiles.world_grid_offset.y ) * Constants::kGridSizePxF.y } );
  draw_world( floor_tiles );
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

void RenderGameSystem::render_wear_level( float wearlevel, const Cmp::Position &pos )
{

  float icon_border = 0.f;
  float padding = 1.f;
  float icon_height = 2.f;
  float icon_width = Constants::kGridSizePxF.x - ( padding * 2 );

  sf::RectangleShape icon( { ( icon_width / 100.f ) * wearlevel, icon_height } );
  icon.setOutlineColor( sf::Color::Black );
  icon.setOutlineThickness( icon_border );
  icon.setFillColor( sf::Color( 255, 0, 0, 224 ) );

  icon.setPosition( { pos.position.x + ( padding ), pos.position.y + Constants::kGridSizePxF.y - icon_height - ( padding ) } );

  draw_world( icon );
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