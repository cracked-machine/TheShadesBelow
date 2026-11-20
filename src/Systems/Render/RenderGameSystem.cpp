
#include <Components/AbsoluteAlpha.hpp>
#include <Components/AbsoluteOffset.hpp>
#include <Components/AbsoluteRenderOffset.hpp>
#include <Components/AbsoluteRotation.hpp>
#include <Components/AbsoluteScale.hpp>
#include <Components/AbsoluteTint.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/Altar/TrailGuide.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armed.hpp>
#include <Components/BurrCount.hpp>
#include <Components/Crypt/BuildingMultiBlock.hpp>
#include <Components/Crypt/Chest.hpp>
#include <Components/Crypt/Entrance.hpp>
#include <Components/Crypt/InteriorMultiBlock.hpp>
#include <Components/Crypt/Lever.hpp>
#include <Components/Crypt/PassageBlock.hpp>
#include <Components/Crypt/RoomLavaPit.hpp>
#include <Components/Exit.hpp>
#include <Components/FractalCurve.hpp>
#include <Components/Grave/Exhumed.hpp>
#include <Components/Grave/ExitMultiBlock.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Grave/TrailMarker.hpp>
#include <Components/Greyscale.hpp>
#include <Components/Hazard/SinkholeCorners.hpp>
#include <Components/Inventory/DowsingTarget.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LastDirection.hpp>
#include <Components/Moveable.hpp>
#include <Components/Npc/Burryman.hpp>
#include <Components/Npc/Crow.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/Shockwave.hpp>
#include <Components/ObstacleCap.hpp>
#include <Components/Persistent/ArmedBlinkFreq.hpp>
#include <Components/Persistent/CameraSmoothSpeed.hpp>
#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Persistent/HeldItemOffset.hpp>
#include <Components/Persistent/HeldItemRotation.hpp>
#include <Components/Persistent/HeldItemRotationSouth.hpp>
#include <Components/Persistent/HeldItemRotationWest.hpp>
#include <Components/Persistent/HeldItemScale.hpp>
#include <Components/Persistent/HeldItemSideOffsetX.hpp>
#include <Components/Persistent/PlayerRenderOffset.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/ArrowCompass.hpp>
#include <Components/Player/Cadaver.hpp>
#include <Components/Player/CameraHold.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/Curse.hpp>
#include <Components/Player/Flying.hpp>
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
#include <Factory/AltarFactory.hpp>
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
#include <Systems/FlyingSystem.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/Render/RenderPassTypes.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Render/UiLayout.hpp>
#include <Systems/ShaderSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Threats/HazardFieldSystemImpl.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Profiling.hpp>
#include <Utils/Utils.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <tracy/Tracy.hpp>
#include <vector>

namespace Game::Sys
{

RenderGameSystem::RenderGameSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : RenderSystem( reg, window, sound_bank )
{
  // the compass arrow avoids the UI panels down the left of the screen, see update_compass_left_inset()
  m_hud_ui_layout = std::make_unique<Render::UiLayout>( "res/ui/ui.json" );
  update_compass_left_inset();

  SPDLOG_DEBUG( "RenderGameSystem initialized" );
}

RenderGameSystem::~RenderGameSystem() = default;

void RenderGameSystem::update_compass_left_inset()
{
  if ( not m_hud_ui_layout ) return;
  m_hud_ui_layout->sync_to( m_window.getSize() );
  // the right edge of the widest ui_outline, in screen pixels at the current UI scale
  const auto &outlines = m_hud_ui_layout->m_outlines;
  const auto widest = std::ranges::max_element( outlines, {}, []( const auto &outline ) { return outline.rect.size.x; } );
  if ( widest != outlines.end() ) m_compass_left_inset_px = widest->rect.position.x + widest->rect.size.x;
}

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
  PROFILED( render_crow_delivery_indicators() );
  PROFILED( render_trail_markers() );

  PROFILED( render_lightning_strike( dt ) );
  PROFILED( render_obstacle_cracks() );

  // lava pit outline
  render_square_for_floatrect_cmp<Cmp::Crypt::RoomLavaPit>( sf::Color( 16, 16, 16 ), 0.5f );
}

void RenderGameSystem::init_world_view()
{
  // init world view dimensions
  s_world_view = sf::View( { kWorldViewSizeF.x * 0.5f, kWorldViewSizeF.y * 0.5f }, kWorldViewSizeF );
  s_world_view.setViewport( sf::FloatRect( { 0.f, 0.f }, { 1.f, 1.f } ) );

  const auto &start_pos = Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( reg() );
  s_world_view.setCenter( start_pos );
}

void RenderGameSystem::render_zorder_queue()
{
  const bool shaders_enabled = Utils::scene_setting<Cmp::SceneSettings::Shaders>( reg() ).enabled;

  // the flying player's sprite is lifted by AbsoluteRenderOffset, so mark their real ground position underneath.
  // It has its own zorder so obstacle caps can hide it.
  std::optional<float> shadow_z;
  if ( Utils::Player::is_flying( reg() ) ) shadow_z = FlyingSystem::shadow_zorder( reg() );

  // render anything with a ZOrderValue component in lowest value first order
  for ( const auto &[z, entity] : s_zorder_queue )
  {
    if ( shadow_z and z > *shadow_z )
    {
      render_flight_shadow();
      shadow_z.reset();
    }

    if ( reg().all_of<Cmp::Position, Cmp::AnimData>( entity ) ) { draw_animated_sprite( entity ); }
    else if ( auto *shader_owner = reg().try_get<Cmp::Shader::SpriteOwner>( entity ) )
    {
      if ( shaders_enabled ) draw_shader_sprite( *shader_owner );
    }
    else if ( auto *particle_owner = reg().try_get<Cmp::Particle::SpriteOwner>( entity ) ) { draw_particle_sprite( *particle_owner ); }
    // armed tile with no sprite (open ground, Cmp::BlastMarker): just the armed indicator. Armed tiles with a sprite
    // draw theirs in draw_animated_sprite()
    else if ( reg().all_of<Cmp::Armed, Cmp::Position>( entity ) )
    {
      render_armed_indicator( reg().get<Cmp::Armed>( entity ), reg().get<Cmp::Position>( entity ) );
    }

    else if ( auto *floor_tiles = reg().try_get<Sprites::Containers::VertexFloor>( entity ) ) { draw_vertex_floor( *floor_tiles ); }
    // the inventory slot holding the dowsing rod: draws the chevrons above the floor and below sprites, only while it is in hand
    else if ( reg().all_of<Cmp::Inventory::DowsingTarget>( entity ) )
    {
      if ( entity == Utils::Player::get_inventory_entt( reg() ) ) { draw_world( Utils::Player::get_doglegs( reg() ).m_markers ); }
    }
    // the chevrons from the activated altar to the trail start graves
    else if ( auto *trail_guide = reg().try_get<Cmp::Altar::TrailGuide>( entity ) ) { draw_world( trail_guide->paths.m_markers ); }
  }
  if ( shadow_z ) render_flight_shadow();
}

void RenderGameSystem::update_camera( sf::Time dt )
{

  // Use the player's current position as the target
  auto target_pos = Utils::Player::get_position( reg() );

  // Initialize camera position on first frame to avoid lerping from origin
  if ( not m_camera_initialized )
  {
    m_camera_position = target_pos.position;
    m_camera_initialized = true;
  }

  // hold the view in place, e.g. while a teleport's departure animation plays
  if ( not reg().view<Cmp::Player::CameraHold>().empty() )
  {
    s_world_view.setCenter( m_camera_position + ( target_pos.size / 2.f ) );
    return;
  }

  // Smooth lerp toward target position
  auto camera_smooth_speed = Sys::PersistSystem::get<Cmp::Persist::CameraSmoothSpeed>( reg() ).get_value();
  float t = 1.0f - std::exp( -camera_smooth_speed * dt.asSeconds() ); // Exponential smoothing

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

bool RenderGameSystem::load_greyscale_shader()
{
  if ( m_greyscale_shader_ok ) return *m_greyscale_shader_ok;

  constexpr const char *kVertPath = "res/shaders/Generic.vert";
  constexpr const char *kFragPath = "res/shaders/Greyscale.frag";
  m_greyscale_shader_ok = false;
  if ( not sf::Shader::isAvailable() ) { SPDLOG_WARN( "Shaders unavailable, greyscale sprites are drawn in colour" ); }
  else if ( not std::filesystem::exists( kVertPath ) or not std::filesystem::exists( kFragPath ) )
  {
    SPDLOG_WARN( "Missing {} or {}, greyscale sprites are drawn in colour", kVertPath, kFragPath );
  }
  else if ( not m_greyscale_shader.loadFromFile( kVertPath, kFragPath ) )
  {
    SPDLOG_WARN( "Failed to load {}, greyscale sprites are drawn in colour", kFragPath );
  }
  else
  {
    m_greyscale_shader.setUniform( "texture", sf::Shader::CurrentTexture );
    m_greyscale_shader_ok = true;
  }
  return *m_greyscale_shader_ok;
}

void RenderGameSystem::draw_animated_sprite( entt::entity entity )
{
  const auto &[pos_cmp, anim_cmp] = reg().get<Cmp::Position, Cmp::AnimData>( entity );

  const auto *alpha_cmp = reg().try_get<Cmp::AbsoluteAlpha>( entity );
  const auto *offset_cmp = reg().try_get<Cmp::AbsoluteOffset>( entity );
  const auto *rotation_cmp = reg().try_get<Cmp::AbsoluteRotation>( entity );
  const auto *tint_cmp = reg().try_get<Cmp::AbsoluteTint>( entity );
  const auto *render_offset_cmp = reg().try_get<Cmp::AbsoluteRenderOffset>( entity );
  const auto *scale_cmp = reg().try_get<Cmp::AbsoluteScale>( entity );

  const uint8_t alpha = alpha_cmp ? alpha_cmp->getAlpha() : 255;
  const sf::Vector2f origin = offset_cmp ? offset_cmp->getOffset() : sf::Vector2f{ 0.f, 0.f };
  const sf::Angle angle = sf::degrees( rotation_cmp ? rotation_cmp->getAngle() : 0.f );
  const sf::Color tint = tint_cmp ? tint_cmp->getTint() : sf::Color::White;
  const sf::Shader *shader = ( reg().all_of<Cmp::Greyscale>( entity ) and load_greyscale_shader() ) ? &m_greyscale_shader : nullptr;
  sf::FloatRect render_rect = pos_cmp;
  if ( render_offset_cmp ) render_rect.position += render_offset_cmp->getOffset();
  // the entity's on-screen bounds, before any mirroring shift below
  const sf::FloatRect body_rect = render_rect;

  // a negative scale mirrors the sprite about its origin, so shift it back to mirror it in place
  sf::Vector2f scale{ 1.f, 1.f };
  if ( anim_cmp.m_flip_x or anim_cmp.m_flip_y )
  {
    const sf::Vector2f sprite_size = Sys::SpriteStore::instance().get( anim_cmp.m_sprite_type ).sprite_size();
    if ( anim_cmp.m_flip_x )
    {
      scale.x = -1.f;
      render_rect.position.x += sprite_size.x - ( 2.f * origin.x );
    }
    if ( anim_cmp.m_flip_y )
    {
      scale.y = -1.f;
      render_rect.position.y += sprite_size.y - ( 2.f * origin.y );
    }
  }

  if ( scale_cmp ) scale *= scale_cmp->getScale();

  // the player's held inventory item: drawn in front of the player, or behind them while they face north
  // const bool is_player = reg().all_of<Cmp::Player::Character>( entity );
  // const auto *facing_cmp = is_player ? reg().try_get<Cmp::LastDirection>( entity ) : nullptr;
  // const sf::Vector2f facing = facing_cmp ? sf::Vector2f( *facing_cmp ) : sf::Vector2f{ 0.f, 0.f };
  // const sf::Vector2f held_item_scale = sf::Vector2f{ 1.f, 1.f } * ( scale_cmp ? scale_cmp->getScale() : 1.f );
  // const bool held_item_behind = facing.y < 0.f;
  // if ( is_player and held_item_behind ) render_held_item( body_rect, facing, held_item_scale, alpha );

  safe_render_sprite_world( anim_cmp.m_sprite_type, render_rect, anim_cmp.getFrameIndexOffset() + anim_cmp.m_current_frame, scale, alpha, origin,
                            angle, tint, shader );

  // per-entity decorations drawn on top of the sprite
  if ( const auto *armed_cmp = reg().try_get<Cmp::Armed>( entity ) ) render_armed_indicator( *armed_cmp, pos_cmp );
  if ( const auto *corners_cmp = reg().try_get<Cmp::Hazard::SinkholeCorners>( entity ) ) render_sinkhole_corners( *corners_cmp, pos_cmp );
  if ( const auto *burr_cmp = reg().try_get<Cmp::BurrCount>( entity ); burr_cmp and reg().all_of<Cmp::Npc::NPC>( entity ) )
  {
    render_attached_burrs( *burr_cmp, body_rect );
  }
  if ( const auto *burryman_cmp = reg().try_get<Cmp::Npc::Burryman>( entity ) ) render_carried_items( *burryman_cmp, body_rect );
  if ( const auto *crow_cmp = reg().try_get<Cmp::Npc::Crow>( entity ) ) render_carried_items( *crow_cmp, body_rect );
  // if ( is_player and not held_item_behind ) render_held_item( body_rect, facing, held_item_scale, alpha );
}

void RenderGameSystem::draw_shader_sprite( Cmp::Shader::SpriteOwner &shader_owner )
{
  if ( not shader_owner.sprite ) return;
  auto &sprite = *shader_owner.sprite;

  if ( sprite.is_sprite_layer() )
  {
    if ( not sprite.active() ) return;
    capture_sprite_layer( sprite );
    draw_screen( sprite );
    return;
  }

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

void RenderGameSystem::capture_sprite_layer( Sprites::IShaderSprite &sprite )
{
  auto &render_texture = sprite.get_render_texture();
  render_texture.clear( sf::Color::Transparent );

  // reuse the z-order queue so the layer keeps the frame's visibility culling and draw order
  set_render_target( render_texture );
  for ( const auto &[z, entity] : s_zorder_queue )
  {
    if ( not reg().any_of<Cmp::Player::Character, Cmp::Npc::NPC, Cmp::WorldItem>( entity ) ) continue;
    if ( reg().all_of<Cmp::Position, Cmp::AnimData>( entity ) ) { draw_animated_sprite( entity ); }
  }
  restore_render_target();
  render_texture.display();
}

void RenderGameSystem::draw_particle_sprite( Cmp::Particle::SpriteOwner &particle_owner )
{
  // screen-space (UI) particles are drawn by HudOverlaySystem and ShopOverlaySystem
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
  static const Sys::ItemKey kNoItem;

  for ( auto [player_entt, pc_cmp, pc_pos_cmp] : reg().view<Cmp::Player::Character, const Cmp::Position>().each() )
  {
    auto &compass = reg().get_or_emplace<Cmp::Player::ArrowCompass>( player_entt );

    // the item in hand (selected slot); read the item type by reference to avoid a per-frame string copy
    const auto inv_entt = Utils::Player::get_inventory_entt( reg() );
    const auto *inv_slot = inv_entt == entt::null ? nullptr : &reg().get<Cmp::PlayerInventorySlot>( inv_entt );
    bool is_offerable = ( inv_slot != nullptr ) and inv_slot->m_item->is_offerable();
    if ( is_offerable )
    {
      // tools can only be offered once spent (see AltarSystem::check_player_altar_offering)
      const auto &type = inv_slot->m_item->item_type;
      if ( type == "item.shovel" )
      {
        const auto *wear_cmp = reg().try_get<Cmp::Inventory::WearLevel>( inv_entt );
        is_offerable = ( wear_cmp != nullptr ) and wear_cmp->m_level <= 0;
      }
    }
    compass.update_mode( inv_slot ? inv_slot->m_item->item_type : kNoItem, is_offerable );

    const sf::Vector2f player_center = pc_pos_cmp.getCenter();
    compass.refresh_targets( reg(), player_center );

    // UI outlines are in screen pixels; scale the inset into world units
    update_compass_left_inset();
    const float left_inset = m_compass_left_inset_px * s_world_view.getSize().x / get_screen_view().getSize().x;

    compass.for_each_needle( [&]( const Cmp::Player::ArrowCompass::Needle &needle )
    {
      auto placement = compass.placement( needle, player_center, s_world_view, left_inset );
      if ( not placement ) return;
      safe_render_sprite_world( needle.sprite, placement->rect, 0, placement->scale, 255, placement->origin, placement->angle );
      if ( not needle.label.empty() ) render_compass_label( needle.label, needle.color, *placement );
    } );
  }
}

sf::Text RenderGameSystem::make_world_label( const std::string &str, sf::Color fill ) const
{
  // rasterise the glyphs large and scale down into world units, so the text stays crisp when the world view is magnified
  constexpr unsigned int kCharSize{ 48 };
  constexpr float kWorldScale{ 0.15f };

  sf::Text text( m_font, str, kCharSize );
  text.setFillColor( fill );
  text.setOutlineColor( sf::Color::Black );
  text.setOutlineThickness( 4.f );
  text.setScale( { kWorldScale, kWorldScale } );
  centre_text_origin( text );
  return text;
}

void RenderGameSystem::centre_text_origin( sf::Text &text )
{
  const sf::FloatRect bounds = text.getLocalBounds();
  text.setOrigin( bounds.position + bounds.size / 2.f );
}

void RenderGameSystem::render_compass_label( std::string_view label, sf::Color color, const Cmp::Player::ArrowCompass::Placement &placement )
{
  auto it = m_compass_label_texts.find( label );
  if ( it == m_compass_label_texts.end() ) { it = m_compass_label_texts.emplace( label, make_world_label( std::string( label ), color ) ).first; }

  // pulled inward from the arrow (back towards the player) so the label doesn't clip at the view edge
  const sf::Vector2f arrow_center = placement.rect.position + placement.origin;
  it->second.setPosition( arrow_center - placement.direction * Constants::kGridSizePxF.x );
  draw_world( it->second );
}

void RenderGameSystem::render_crow_delivery_indicators()
{
  auto crow_view = reg().view<Cmp::Npc::Crow, Cmp::Position>();
  if ( crow_view.begin() == crow_view.end() ) return;

  const sf::Vector2f player_center = Utils::Player::get_position( reg() ).getCenter();
  // UI outlines are in screen pixels; scale the inset into world units (m_compass_left_inset_px is kept current by render_arrow_compass)
  const float left_inset = m_compass_left_inset_px * s_world_view.getSize().x / get_screen_view().getSize().x;

  for ( auto [crow_entt, crow_cmp, crow_pos_cmp] : crow_view.each() )
  {
    // only couriers still flying in with their item
    if ( crow_cmp.carried == entt::null or crow_cmp.flee or not reg().valid( crow_cmp.carried ) ) continue;
    const auto *item_cmp = reg().try_get<Cmp::WorldItem>( crow_cmp.carried );
    if ( not item_cmp ) continue;

    // hidden once the crow is in view
    const auto edge = Cmp::Player::ArrowCompass::edge_point( crow_pos_cmp, player_center, s_world_view, left_inset );
    if ( not edge ) continue;

    const sf::Vector2f drop_centre = crow_cmp.drop_position + Constants::kGridSizePxF / 2.f;
    const int tiles_left = static_cast<int>( std::ceil( ( drop_centre - crow_pos_cmp.getCenter() ).length() / Constants::kGridSizePxF.x ) );
    const std::string_view key_name = item_cmp->item_type.contains( "exitkey" ) ? "exit key" : item_cmp->item_type.contains( "cryptkey" ) ? "crypt key" : "item";
    const std::string str = std::format( "{} {}", key_name, tiles_left );

    auto it = m_crow_delivery_texts.find( item_cmp->item_type.str() );
    if ( it == m_crow_delivery_texts.end() )
    {
      it = m_crow_delivery_texts.emplace( item_cmp->item_type.str(), make_world_label( str, sf::Color::White ) ).first;
    }
    else if ( it->second.getString() != str )
    {
      // only regenerate the glyph geometry when the count changes
      it->second.setString( str );
      centre_text_origin( it->second );
    }
    it->second.setPosition( edge->first );
    draw_world( it->second );
  }
}

void RenderGameSystem::render_trail_markers()
{
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );
  const sf::Vector2f half_grid = Constants::kGridSizePxF / 2.f;
  // an un-exhumed marker is the hidden target grave, so only the exhumed marker (GraveSystem keeps just the latest) shows an arrow
  for ( auto [grave_entt, marker_cmp, grave_pos_cmp] : reg().view<Cmp::Grave::TrailMarker, Cmp::Position, Cmp::Grave::Exhumed>().each() )
  {
    if ( not Utils::is_visible_in_view( view_bounds, grave_pos_cmp ) ) continue;

    // the origin is the sprite centre, so the arrow is turned in place over the middle of the grave
    safe_render_sprite_world( "sprite.graveyard.arrowtrail", { grave_pos_cmp.getCenter() - half_grid, Constants::kGridSizePxF }, 0, { 1.f, 1.f }, 255,
                              half_grid, marker_cmp.angle );
  }
}

void RenderGameSystem::render_attached_burrs( const Cmp::BurrCount &burr_cmp, const sf::FloatRect &body_rect )
{
  const sf::Vector2f half_cell = Constants::kGridSizePxF / 2.f;
  const sf::Vector2f centre = body_rect.getCenter();

  // each burr is drawn on the face where it struck the NPC (see Sys::BurrSystem), so it follows the NPC as it moves
  for ( const auto &burr : burr_cmp.burrs() )
  {
    // the origin is the sprite centre, so the burr is scaled and turned to face outwards in place
    safe_render_sprite_world( "sprite.burr", sf::FloatRect( centre + burr.offset - half_cell, Constants::kGridSizePxF ), 0,
                              { Cmp::BurrCount::kStuckScale, Cmp::BurrCount::kStuckScale }, 255, half_cell, burr.side.angle() );
  }
}

void RenderGameSystem::render_carried_items( const Cmp::Npc::Burryman &burryman_cmp, const sf::FloatRect &body_rect )
{
  const sf::Vector2f half_cell = Constants::kGridSizePxF / 2.f;
  const sf::Vector2f centre = body_rect.getCenter();
  const std::size_t count = burryman_cmp.carried.size();

  for ( std::size_t i = 0; i < count; ++i )
  {
    // the item may have worn away while he carried it
    if ( not reg().valid( burryman_cmp.carried[i] ) ) continue;
    const auto *item_cmp = reg().try_get<Cmp::WorldItem>( burryman_cmp.carried[i] );
    if ( not item_cmp ) continue;
    // the origin is the sprite centre, so the item is scaled down in place
    const sf::Vector2f offset = Cmp::Npc::Burryman::carried_offset( i, count, body_rect.size );
    safe_render_sprite_world( item_cmp->sprite_type, sf::FloatRect( centre + offset - half_cell, Constants::kGridSizePxF ),
                              carried_item_frame( burryman_cmp.carried[i], *item_cmp ),
                              { Cmp::Npc::Burryman::kCarriedScale, Cmp::Npc::Burryman::kCarriedScale }, 255, half_cell );
  }
}

std::size_t RenderGameSystem::carried_item_frame( entt::entity item_entt, const Cmp::WorldItem &item_cmp )
{
  if ( const auto *anim_cmp = reg().try_get<Cmp::AnimData>( item_entt ) ) { return anim_cmp->getFrameIndexOffset(); }
  return item_cmp.icon_sprite_index();
}

void RenderGameSystem::render_carried_items( const Cmp::Npc::Crow &crow_cmp, const sf::FloatRect &body_rect )
{
  if ( crow_cmp.carried == entt::null or not reg().valid( crow_cmp.carried ) ) return;
  const auto *item_cmp = reg().try_get<Cmp::WorldItem>( crow_cmp.carried );
  if ( not item_cmp ) return;
  // the origin is the sprite centre, so the item is scaled down in place, centred on the crow's bottom edge
  const sf::Vector2f half_cell = Constants::kGridSizePxF / 2.f;
  const sf::Vector2f hang_point( body_rect.getCenter().x, body_rect.position.y + body_rect.size.y );
  safe_render_sprite_world( item_cmp->sprite_type, sf::FloatRect( hang_point - half_cell, Constants::kGridSizePxF ),
                            carried_item_frame( crow_cmp.carried, *item_cmp ),
                            { Cmp::Npc::Crow::kCarriedScale, Cmp::Npc::Crow::kCarriedScale }, 255, half_cell );
}

void RenderGameSystem::render_held_item( const sf::FloatRect &body_rect, sf::Vector2f facing, sf::Vector2f scale, uint8_t alpha )
{
  sf::Vector2f offset = Sys::PersistSystem::get<Cmp::Persist::HeldItemOffset>( reg() );
  // facing east/west: shift the item towards the facing side
  if ( facing.x != 0.f )
  {
    offset.x += ( facing.x > 0.f ? 1.f : -1.f ) * Sys::PersistSystem::get<Cmp::Persist::HeldItemSideOffsetX>( reg() ).get_value();
  }
  // facing west: mirror the item about its centre rather than only rotating it, so its art isn't upside down
  const bool facing_west = facing.x < 0.f;
  float rotation_deg = Sys::PersistSystem::get<Cmp::Persist::HeldItemRotation>( reg() ).get_value();
  if ( facing_west )
    rotation_deg = Sys::PersistSystem::get<Cmp::Persist::HeldItemRotationWest>( reg() ).get_value();
  else if ( facing.y > 0.f )
    rotation_deg = Sys::PersistSystem::get<Cmp::Persist::HeldItemRotationSouth>( reg() ).get_value();
  const sf::Angle rotation = sf::degrees( rotation_deg );
  if ( facing_west ) scale.x = -scale.x;
  scale *= Sys::PersistSystem::get<Cmp::Persist::HeldItemScale>( reg() ).get_value();

  // only the item in hand (selected slot); same sprite as its HUD slot (see HudOverlaySystem::render_ui_inventory_table)
  const auto inventory_entt = Utils::Player::get_inventory_entt( reg() );
  if ( inventory_entt == entt::null ) return;
  if ( const auto *anim_cmp = reg().try_get<Cmp::AnimData>( inventory_entt ) )
  {
    // the origin is the sprite centre, so the item is scaled and tilted in place
    safe_render_sprite_world( anim_cmp->m_sprite_type, sf::FloatRect( body_rect.position + offset, Constants::kGridSizePxF ), 0, scale, alpha,
                              Constants::kGridSizePxF / 2.f, rotation );
  }
}

void RenderGameSystem::render_flight_shadow()
{
  // a flattened circle filling the shadow bounds
  const float radius = FlyingSystem::kFlightShadowRadiusPx;
  sf::CircleShape shadow( radius );
  shadow.setOrigin( { radius, radius } );
  shadow.setScale( { 1.f, 0.5f } );
  shadow.setPosition( FlyingSystem::shadow_bounds( reg() ).getCenter() );
  shadow.setFillColor( sf::Color( 0, 0, 0, FlyingSystem::kFlightShadowAlpha ) );
  draw_world( shadow );
}

void RenderGameSystem::render_sinkhole_corners( const Cmp::Hazard::SinkholeCorners &corners_cmp, const Cmp::Position &pos_cmp )
{
  static const std::array<std::pair<uint8_t, Sys::SpriteKey>, 4> corner_sprites{ {
      { Cmp::Hazard::SinkholeCorners::kNorthEast, "sprite.graveyard.hazard.sinkhole_corner_north_east" },
      { Cmp::Hazard::SinkholeCorners::kNorthWest, "sprite.graveyard.hazard.sinkhole_corner_north_west" },
      { Cmp::Hazard::SinkholeCorners::kSouthEast, "sprite.graveyard.hazard.sinkhole_corner_south_east" },
      { Cmp::Hazard::SinkholeCorners::kSouthWest, "sprite.graveyard.hazard.sinkhole_corner_south_west" },
  } };
  for ( const auto &[corner, sprite_key] : corner_sprites )
  {
    if ( corners_cmp.mask & corner ) safe_render_sprite_world( sprite_key, pos_cmp, 0 );
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
    const float &flash_clk_elapsed_secs = Utils::Player::get_global_bomb_flash_timer( reg() ).asSeconds();
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

void RenderGameSystem::render_lightning_strike( sf::Time dt )
{

  for ( auto [entt, cmp] : reg().view<Cmp::LightningStrike>().each() )
  {
    cmp.timer += dt;
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

  const auto &display_res = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );
  auto flash = sf::RectangleShape( sf::Vector2f( display_res ) );
  flash.setPosition( { 0.f, 0.f } );
  flash.setFillColor( color );
  // draw flash in screen view so it covers the whole screen in screen-space
  draw_screen( flash );
}

} // namespace Game::Sys