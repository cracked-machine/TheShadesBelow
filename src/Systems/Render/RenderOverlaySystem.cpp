#include <Components/AnimData.hpp>
#include <Components/Crypt/ShuffleTimer.hpp>
#include <Components/Grave/Consequence.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Hazard/FieldCell.hpp>
#include <Components/Inventory/FlashUICadaver.hpp>
#include <Components/Inventory/FlashUIExtraLife.hpp>
#include <Components/Inventory/FlashUIHealth.hpp>
#include <Components/Inventory/FlashUIInventory.hpp>
#include <Components/Inventory/FlashUIRadius.hpp>
#include <Components/Inventory/FlashUIWealth.hpp>
#include <Components/Inventory/Grimoire.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/LastDirection.hpp>
#include <Components/NoRender.hpp>
#include <Components/Persistent/CryptShuffleTimeout.hpp>
#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Persistent/ToxicImmunityMeterFill.hpp>
#include <Components/Persistent/ToxicImmunityMeterShimmer.hpp>
#include <Components/Persistent/ToxicImmunityMeterTint.hpp>
#include <Components/Player/BlastRadius.hpp>
#include <Components/Player/CadaverCount.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Player/ToxicImmunity.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/Shop/Inventory.hpp>
#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Components/Toxicity/Venom.hpp>
#include <Components/Toxicity/Vertigo.hpp>
#include <Components/Wall.hpp>
#include <Components/ZOrderValue.hpp>
#include <Factory/ParticleFactory.hpp>
#include <SFML/System/Time.hpp>
#include <SceneControl/Scenes/CryptScene.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderOverlaySystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Crypt.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Profiling.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <string>
#include <utility>

namespace Game::Sys
{

void RenderOverlaySystem::render_overlay( sf::Time dt )
{
  PROFILED( render_ui_particles() );

  PROFILED( render_shop_inventory_overlay() );
  PROFILED( render_ui_outlines() );
  PROFILED( render_ui_icons() );
  PROFILED( render_ui_inventory_icon() );
  PROFILED( render_ui_meters( dt ) );
  PROFILED( render_ui_labels( dt ) );
  PROFILED( render_ui_texts() );
  PROFILED( render_level_depth() );
  PROFILED( render_ghost_grave_halos() );

  const auto &display_size = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );
  render_crypt_maze_timer( { static_cast<float>( display_size.x ) / 2.f, 0.f }, 100 );
}

void RenderOverlaySystem::render_ui_particles()
{
  if ( not m_main_ui_data ) return;

  // only candles are moved into screen-space, they flicker over the inventory icon
  auto view = reg().view<Cmp::Particle::SpriteOwner>( entt::exclude<Cmp::NoRender> );
  for ( auto [entity, particle_owner] : view.each() )
  {
    if ( not particle_owner.sprite ) continue;
    auto &sprite = *particle_owner.sprite;
    if ( sprite.get_view_type() != Cmp::Particle::ViewType::SCREEN ) continue;
    if ( not sprite.get_tag().contains( "candle" ) ) continue;

    sprite.set_view_transform( m_window, m_window.getDefaultView() );
    for ( auto &icon : m_main_ui_data->m_icons )
    {
      if ( icon.name != "inventory_icon" ) continue;
      sprite.set_emitter_position( { icon.rect.position.x + ( icon.scale * 8.f ), icon.rect.position.y + ( icon.scale * 6.f ) } );
    }
    sprite.restart();
    draw_screen( sprite );
  }
}

void RenderOverlaySystem::render_ui_outlines()
{
  if ( not m_main_ui_data )
  {
    SPDLOG_CRITICAL( "UiData object is not initialised. Cannot draw Status Outline overlay" );
    return;
  }
  render_outlines( m_main_ui_data->m_outlines );
}

void RenderOverlaySystem::render_ui_icons()
{
  if ( not m_main_ui_data )
  {
    SPDLOG_CRITICAL( "UiData object is not initialised. Cannot draw icon overlay" );
    return;
  }
  for ( const auto &icon : m_main_ui_data->m_icons )
  {
    // we handle the inventory icon seperately from the others
    if ( icon.name == "inventory_icon" ) continue;
    RenderSystem::safe_render_sprite_screen( icon.type, icon.rect, icon.index,
                                             { static_cast<float>( icon.scale ), static_cast<float>( icon.scale ) } );
  }
}

void RenderOverlaySystem::render_ui_inventory_icon()
{
  if ( not m_main_ui_data )
  {
    SPDLOG_CRITICAL( "UiData object is not initialised. Cannot draw icon overlay" );
    return;
  }
  for ( const auto &icon : m_main_ui_data->m_icons )
  {

    if ( icon.name != "inventory_icon" ) continue;

    auto inventory_view = reg().view<Cmp::PlayerInventorySlot, Cmp::AnimData>();
    for ( auto [inventory_entt, inventory_cmp, anim_cmp] : inventory_view.each() )
    {
      RenderSystem::safe_render_sprite_screen( anim_cmp.m_sprite_type, { icon.rect.position, Constants::kGridSizePxF }, 0,
                                               { static_cast<float>( icon.scale ), static_cast<float>( icon.scale ) } );
    }
  }
}

void RenderOverlaySystem::render_ui_meters( sf::Time dt )
{
  if ( not m_main_ui_data )
  {
    SPDLOG_CRITICAL( "UiData object is not initialised. Cannot draw icon overlay" );
    return;
  }
  // toxidrome meters get a frozen fill and a frosted overlay while the player is immune, blinking as the immunity runs out
  const auto *immunity_cmp = reg().try_get<Cmp::Player::ToxicImmunity>( Utils::Player::get_entity( reg() ) );
  const bool immunity_visible = immunity_cmp and immunity_cmp->indicator_visible();
  const sf::Color toxidrome_fill = immunity_visible ? sf::Color( Sys::PersistSystem::get<Cmp::Persist::ToxicImmunityMeterFill>( reg() ) )
                                                    : sf::Color::Green;
  const sf::Color toxidrome_overlay = Sys::PersistSystem::get<Cmp::Persist::ToxicImmunityMeterTint>( reg() );
  const sf::Color toxidrome_glint = Sys::PersistSystem::get<Cmp::Persist::ToxicImmunityMeterShimmer>( reg() );
  // gives each frozen meter its own frost pattern
  int toxidrome_index = 0;
  constexpr float kMeterOutlineThickness = 5.f;

  sf::RectangleShape innermeter;
  sf::RectangleShape outermeter;
  for ( const auto &meter : m_main_ui_data->m_meters )
  {
    float meter_value = 0;
    sf::Color meter_inner_color;
    sf::Color meter_outer_color = sf::Color::Black;
    bool should_render = false;
    bool is_toxidrome_meter = false;

    if ( meter.name == "health_meter" )
    {
      meter_value = static_cast<float>( Utils::Player::get_stats( reg() ).health() );
      meter_inner_color = sf::Color::Red;
      should_render = true;

      if ( update_flash_toggle<Cmp::FlashUIHealth>( dt ) ) { meter_outer_color = sf::Color::Cyan; }
      if ( update_flash_toggle<Cmp::FlashUIExtraLife>( dt ) ) { meter_inner_color = sf::Color::Magenta; }
      else { meter_inner_color = sf::Color::Red; }
    }
    else if ( meter.name == "fear_meter" )
    {
      meter_value = static_cast<float>( Utils::Player::get_stats( reg() ).fear() );
      meter_inner_color = sf::Color::Yellow;
      should_render = true;
    }
    else if ( meter.name == "infamy_meter" )
    {
      meter_value = static_cast<float>( Utils::Player::get_stats( reg() ).infamy() );
      meter_inner_color = sf::Color::Magenta;
      should_render = true;
    }
    else if ( meter.name == "despair_meter" )
    {
      meter_value = static_cast<float>( Utils::Player::get_stats( reg() ).despair() );
      meter_inner_color = sf::Color::Blue;
      should_render = true;
    }
    else if ( meter.name == "luck_meter" )
    {
      meter_value = static_cast<float>( Utils::Player::get_stats( reg() ).luck() );
      meter_inner_color = sf::Color::Cyan;
      should_render = true;
    }
    else if ( meter.name == "tachycardia_meter" )
    {
      auto opt_meter = Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Tachycardia>();
      meter_value = static_cast<float>( opt_meter.value_or( 0 ) );
      meter_inner_color = toxidrome_fill;
      meter_outer_color = sf::Color( 64, 64, 64, 255 );
      is_toxidrome_meter = true;
      should_render = true;
    }
    else if ( meter.name == "bradycardia_meter" )
    {
      auto opt_meter = Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Bradycardia>();
      meter_value = static_cast<float>( opt_meter.value_or( 0 ) );
      meter_inner_color = toxidrome_fill;
      meter_outer_color = sf::Color( 64, 64, 64, 255 );
      is_toxidrome_meter = true;
      should_render = true;
    }
    else if ( meter.name == "hallucinogen_meter" )
    {
      auto opt_meter = Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Hallucinogen>();
      meter_value = static_cast<float>( opt_meter.value_or( 0 ) );
      meter_inner_color = toxidrome_fill;
      meter_outer_color = sf::Color( 64, 64, 64, 255 );
      is_toxidrome_meter = true;
      should_render = true;
    }
    else if ( meter.name == "hypoxia_meter" )
    {
      auto opt_meter = Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Hypoxia>();
      meter_value = static_cast<float>( opt_meter.value_or( 0 ) );
      meter_inner_color = toxidrome_fill;
      meter_outer_color = sf::Color( 64, 64, 64, 255 );
      is_toxidrome_meter = true;
      should_render = true;
    }
    else if ( meter.name == "phototoxia_meter" )
    {
      auto opt_meter = Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Phototoxia>();
      meter_value = static_cast<float>( opt_meter.value_or( 0 ) );
      meter_inner_color = toxidrome_fill;
      meter_outer_color = sf::Color( 64, 64, 64, 255 );
      is_toxidrome_meter = true;
      should_render = true;
    }
    else if ( meter.name == "venom_meter" )
    {
      auto opt_meter = Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Venom>();
      meter_value = static_cast<float>( opt_meter.value_or( 0 ) );
      meter_inner_color = toxidrome_fill;
      meter_outer_color = sf::Color( 64, 64, 64, 255 );
      is_toxidrome_meter = true;
      should_render = true;
    }
    else if ( meter.name == "vertigo_meter" )
    {
      auto opt_meter = Utils::Player::get_stats( reg() ).toxidrome().at<Cmp::Toxicity::Vertigo>();
      meter_value = static_cast<float>( opt_meter.value_or( 0 ) );
      meter_inner_color = toxidrome_fill;
      meter_outer_color = sf::Color( 64, 64, 64, 255 );
      is_toxidrome_meter = true;
      should_render = true;
    }
    else if ( meter.name == "inventory_meter" )
    {
      meter_value = Utils::Player::get_inventory_wear_level( reg() );
      if ( meter_value >= 0 ) { should_render = true; }
      meter_inner_color = sf::Color::Red;
    }

    if ( not should_render ) { continue; }

    innermeter.setSize( { ( ( meter.rect.size.x / 100 ) * meter_value ), meter.rect.size.y } );
    innermeter.setPosition( meter.rect.position );
    innermeter.setFillColor( meter_inner_color );
    draw_screen( innermeter );

    outermeter.setSize( meter.rect.size );
    outermeter.setPosition( meter.rect.position );
    outermeter.setFillColor( sf::Color::Transparent );
    outermeter.setOutlineColor( meter_outer_color );
    outermeter.setOutlineThickness( kMeterOutlineThickness );
    draw_screen( outermeter );

    // frost the whole toxidrome meter, outline included, so even an empty meter shows the immunity
    if ( immunity_visible and is_toxidrome_meter )
    {
      const sf::Vector2f outline( kMeterOutlineThickness, kMeterOutlineThickness );
      const sf::FloatRect overlay_rect( meter.rect.position - outline, meter.rect.size + outline * 2.f );
      if ( load_frost_shader() )
      {
        render_meter_frost( overlay_rect, static_cast<float>( toxidrome_index ) * 17.31f, toxidrome_overlay, toxidrome_glint );
      }
      else
      {
        // no shader support: fall back to a flat frost wash
        sf::RectangleShape overlay( overlay_rect.size );
        overlay.setPosition( overlay_rect.position );
        overlay.setFillColor( toxidrome_overlay );
        draw_screen( overlay );
      }
      ++toxidrome_index;
    }
  }
}

void RenderOverlaySystem::render_meter_shimmer( const sf::FloatRect &rect, float progress, sf::Color colour )
{
  constexpr float kShimmerHalfWidth = 10.f;
  // the band leans like "/": its top edge is offset right of its bottom edge by the slant
  const float half_slant = rect.size.y * 0.25f;
  const float left = rect.position.x;
  const float right = rect.position.x + rect.size.x;
  const float top = rect.position.y;
  const float bottom = rect.position.y + rect.size.y;

  // the band centre travels from fully off the left edge to fully off the right edge
  const float travel = kShimmerHalfWidth + half_slant;
  const float centre = ( left - travel ) + ( ( right - left ) + ( travel * 2.f ) ) * progress;

  sf::Color edge_colour = colour;
  edge_colour.a = 0;

  // trailing, centre and leading slanted edges; clamping x keeps the band inside the meter
  sf::VertexArray band( sf::PrimitiveType::TriangleStrip, 6 );
  const std::array<std::pair<float, sf::Color>, 3> edges = {
      { { centre - kShimmerHalfWidth, edge_colour }, { centre, colour }, { centre + kShimmerHalfWidth, edge_colour } } };
  for ( std::size_t idx = 0; idx < edges.size(); ++idx )
  {
    const auto &[edge_x, edge_colour_at] = edges[idx];
    band[idx * 2] = sf::Vertex{ { std::clamp( edge_x + half_slant, left, right ), top }, edge_colour_at };
    band[( idx * 2 ) + 1] = sf::Vertex{ { std::clamp( edge_x - half_slant, left, right ), bottom }, edge_colour_at };
  }
  draw_screen( band );
}

bool RenderOverlaySystem::load_frost_shader()
{
  if ( m_frost_shader_ok ) return *m_frost_shader_ok;

  constexpr const char *kVertPath = "res/shaders/Generic.vert";
  constexpr const char *kFragPath = "res/shaders/Frost.frag";
  m_frost_shader_ok = false;
  if ( not sf::Shader::isAvailable() ) { SPDLOG_WARN( "Shaders unavailable, toxidrome meters use a flat frost overlay" ); }
  else if ( not std::filesystem::exists( kVertPath ) or not std::filesystem::exists( kFragPath ) )
  {
    SPDLOG_WARN( "Missing {} or {}, toxidrome meters use a flat frost overlay", kVertPath, kFragPath );
  }
  else if ( not m_frost_shader.loadFromFile( kVertPath, kFragPath ) )
  {
    SPDLOG_WARN( "Failed to load {}, toxidrome meters use a flat frost overlay", kFragPath );
  }
  else { m_frost_shader_ok = true; }
  return *m_frost_shader_ok;
}

void RenderOverlaySystem::render_meter_frost( const sf::FloatRect &rect, float seed, sf::Color frost, sf::Color glint )
{
  // the shader works in render target pixels (gl_FragCoord), so map the default view rect onto the target
  auto &target = active_render_target();
  const sf::View &screen_view = m_window.getDefaultView();
  const sf::Vector2f top_left = sf::Vector2f( target.mapCoordsToPixel( rect.position, screen_view ) );
  const sf::Vector2f bottom_right = sf::Vector2f( target.mapCoordsToPixel( rect.position + rect.size, screen_view ) );

  m_frost_shader.setUniform( "rect", sf::Glsl::Vec4( top_left.x, top_left.y, bottom_right.x - top_left.x, bottom_right.y - top_left.y ) );
  m_frost_shader.setUniform( "target_height", static_cast<float>( target.getSize().y ) );
  m_frost_shader.setUniform( "time", m_frost_clock.getElapsedTime().asSeconds() );
  m_frost_shader.setUniform( "seed", seed );
  m_frost_shader.setUniform( "frost", sf::Glsl::Vec4( frost ) );
  m_frost_shader.setUniform( "glint", sf::Glsl::Vec4( glint ) );

  sf::RectangleShape overlay( rect.size );
  overlay.setPosition( rect.position );
  overlay.setFillColor( sf::Color::White );
  draw_screen( overlay, sf::RenderStates( &m_frost_shader ) );
}

void RenderOverlaySystem::render_ui_texts()
{
  if ( not m_main_ui_data )
  {
    SPDLOG_CRITICAL( "UiData object is not initialised. Cannot draw value overlay" );
    return;
  }

  sf::Text text( m_font, "", 1 );
  for ( const auto &ui_text : m_main_ui_data->m_texts )
  {
    text.setString( ui_text.value );
    text.setCharacterSize( ui_text.font_size );
    text.setPosition( ui_text.rect.position );
    text.setFillColor( sf::Color::White );
    text.setOutlineColor( sf::Color::Black );
    text.setOutlineThickness( 2.f );
    draw_screen( text );
  }
}

//! @brief Advances `interval` by `dt` while a `FlashComponent` is present on any entity, removing it once its duration has elapsed.
//! Returns whether the UI element should currently be drawn in its "flashed" state (toggles on/off at m_ui_flash_factor ms).
//! @tparam FlashComponent
//! @param dt
//! @param interval accumulated flash time; reset to zero once the flash expires
//! @return true if the UI element should currently be drawn in its "flashed" state, false otherwise.
template <typename FlashComponent>
bool RenderOverlaySystem::update_flash_toggle( sf::Time dt )
{
  auto view = reg().view<FlashComponent>();
  if ( view.empty() ) return false;

  auto flash_entt = view.front();
  auto &flash_cmp = view.template get<FlashComponent>( flash_entt );
  flash_cmp.cooldown_timer += dt;
  if ( flash_cmp.timeout() != sf::Time::Zero and flash_cmp.cooldown_timer >= flash_cmp.timeout() )
  {
    reg().remove<FlashComponent>( flash_entt );
    flash_cmp.cooldown_timer = sf::Time::Zero;
    return false;
  }
  return static_cast<int>( flash_cmp.cooldown_timer.asMilliseconds() / m_ui_flash_factor ) % 2 == 1;
}

void RenderOverlaySystem::render_ui_labels( sf::Time dt )
{
  if ( not m_main_ui_data )
  {
    SPDLOG_CRITICAL( "UiData object is not initialised. Cannot draw value overlay" );
    return;
  }
  sf::Text text( m_font, "", 1 );
  for ( const auto &ui_label : m_main_ui_data->m_labels )
  {
    std::string text_str;
    if ( ui_label.name == "radius_label" ) { text_str = " =   " + std::to_string( Utils::Player::get_blast_radius( reg() ).value ); }
    else if ( ui_label.name == "cadaver_label" ) { text_str = " =   " + std::to_string( Utils::Player::get_cadaver_count( reg() ).get_count() ); }
    else if ( ui_label.name == "wealth_label" ) { text_str = " =   " + std::to_string( Utils::Player::get_wealth( reg() ).wealth ); }
    else if ( ui_label.name == "inventory_label" )
    {
      auto [_, _, inventory_sprite_type] = Utils::Player::get_inventory( reg() );
      if ( inventory_sprite_type == "" ) { text_str = ""; }
      else { text_str = Sys::SpriteStore::instance().get( inventory_sprite_type ).display_name(); }
    }
    text.setCharacterSize( ui_label.font_size );
    text.setString( text_str );
    if ( ui_label.align == "center" )
    {
      text.setPosition( { ui_label.rect.position.x - text.getLocalBounds().getCenter().x, ui_label.rect.position.y } );
    }
    else if ( ui_label.align == "left" ) { text.setPosition( ui_label.rect.position ); }
    else if ( ui_label.align == "right" )
    {
      text.setPosition( { ui_label.rect.position.x - text.getLocalBounds().size.x, ui_label.rect.position.y } );
    }
    text.setFillColor( sf::Color::White );
    text.setOutlineColor( sf::Color::Black );
    text.setOutlineThickness( 2.f );

    // flash the text if we just increased the bomb blast radius
    if ( ui_label.name == "radius_label" and update_flash_toggle<Cmp::FlashUIRadius>( dt ) )
    {
      text.setFillColor( sf::Color::White );
      text.setOutlineColor( sf::Color::White );
    }

    // flash the text if we just picked up a cadaver
    if ( ui_label.name == "cadaver_label" and update_flash_toggle<Cmp::FlashUICadaver>( dt ) )
    {
      text.setFillColor( sf::Color::White );
      text.setOutlineColor( sf::Color::White );
    }

    // flash the text if we just deposited something in a well
    if ( ui_label.name == "wealth_label" and update_flash_toggle<Cmp::FlashUIWealth>( dt ) )
    {
      text.setFillColor( sf::Color::White );
      text.setOutlineColor( sf::Color::White );
    }

    // flash the text if we just picked up a Key
    if ( ui_label.name == "inventory_label" and update_flash_toggle<Cmp::FlashUIInventory>( dt ) )
    {
      text.setFillColor( sf::Color::Black );
      text.setOutlineColor( sf::Color::White );
    }

    draw_screen( text );
  }
}

void RenderOverlaySystem::render_level_depth()
{
  const auto &player_level_cmp = Utils::Player::get_level_depth( reg() );
  if ( player_level_cmp.display_timer.getElapsedTime() >= player_level_cmp.display_cooldown ) { return; }

  const auto &display_res = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );
  sf::Text level_txt( m_font, "Nekropolis " + std::to_string( player_level_cmp.get_count() ), 100 );
  level_txt.setPosition( sf::Vector2f( ( static_cast<float>( display_res.x ) / 2.f ) - level_txt.getLocalBounds().getCenter().x, 100.f ) );
  level_txt.setFillColor( sf::Color::White );
  draw_screen( level_txt );
}

void RenderOverlaySystem::render_shop_inventory_overlay()
{

  // only draw the shop if its enabled
  auto inventory_view = reg().view<Cmp::Shop::Inventory>();
  if ( inventory_view.empty() ) { return; }
  auto &inventory_cmp = inventory_view.get<Cmp::Shop::Inventory>( inventory_view.front() );
  if ( not inventory_cmp.is_enabled ) return;

  if ( not m_shop_ui_data )
  {
    SPDLOG_CRITICAL( "UiData object is not initialised. Cannot draw value overlay" );
    return;
  }

  // Draw all UI outlines
  render_outlines( m_shop_ui_data->m_outlines );

  // The shop flames are an unassigned pool (see Factory::Particle::sync_flames_for_shop_inventory):
  // each candle slot just takes the next unused one.
  auto flame_view = reg().view<Cmp::Particle::SpriteOwner>();
  auto flame_it = flame_view.begin();
  auto next_slot_flame = [&]() -> Cmp::Particle::SpriteOwner *
  {
    while ( flame_it != flame_view.end() )
    {
      auto &owner = flame_view.get<Cmp::Particle::SpriteOwner>( *flame_it );
      ++flame_it;
      if ( owner.sprite and owner.sprite->get_tag() == Factory::Particle::kShopSlotFlameTag ) return &owner;
    }
    return nullptr;
  };

  // Draw all UI Icons
  for ( auto [icon, slot] : std::views::zip( m_shop_ui_data->m_icons, inventory_cmp.m_slots ) )
  {
    auto &[item, price] = slot;
    auto sprite_type = Sys::ItemStore::instance().get( item ).sprite_type;
    // Use the default scale/size unless its a plant then its need to be resized/repositioned to fit in the UI box
    auto sprite_scale = sf::Vector2f{ static_cast<float>( icon.scale ), static_cast<float>( icon.scale ) };
    auto sprite_pos = Cmp::Position( icon.rect.position, Constants::kGridSizePxF );
    if ( sprite_type.contains( "plant" ) )
    {
      sprite_scale = sf::Vector2f{ static_cast<float>( icon.scale ), static_cast<float>( icon.scale - 1 ) };
      sprite_pos = Cmp::Position( { icon.rect.position.x, icon.rect.position.y - 8 }, Constants::kGridSizePxF );
    }

    RenderSystem::safe_render_sprite_screen( sprite_type, sprite_pos, 0, sprite_scale );

    if ( item != "item.candle" ) continue;
    auto *flame_owner = next_slot_flame();
    if ( not flame_owner ) continue;
    auto &flame = *flame_owner->sprite;
    // same wick offset as the player inventory icon in render_ui_particles()
    const sf::Vector2f emitter_pos{ icon.rect.position.x + ( icon.scale * 8.f ), icon.rect.position.y + ( icon.scale * 6.f ) };
    if ( flame.get_emitter_position() != emitter_pos )
    {
      // slots shift when an item is bought: drop the particles still rising from the old position
      flame.set_emitter_position( emitter_pos );
      flame.clear();
    }
    flame.set_view_transform( m_window, m_window.getDefaultView() );
    flame.restart();
    draw_screen( flame );
  }

  //! @brief Helper to draw predefined `sf_text` at `pos`
  auto draw_label_text_at = [&]( sf::Text &sf_text, const sf::Vector2f &pos, const std::string &align = "left" )
  {
    if ( align == "center" ) { sf_text.setPosition( { pos.x - sf_text.getLocalBounds().getCenter().x, pos.y } ); }
    else if ( align == "left" ) { sf_text.setPosition( pos ); }
    else if ( align == "right" ) { sf_text.setPosition( { pos.x - sf_text.getLocalBounds().size.x, pos.y } ); }

    draw_screen( sf_text );
  };

  // Draw all text labels (index, description, price)
  for ( auto [i, slot] : std::views::enumerate( inventory_cmp.m_slots ) )
  {
    auto &[item, price] = slot;

    sf::Text slot_idx_txt( m_font, std::to_string( i + 1 ), 30 );
    slot_idx_txt.setFillColor( sf::Color::Black );

    Sys::SpriteKey sprite_mtype = Sys::ItemStore::instance().get( item ).sprite_type;
    sf::Text slot_desc_txt( m_font, Sys::SpriteStore::instance().get( sprite_mtype ).display_name(), 30 );
    slot_desc_txt.setFillColor( sf::Color::Black );

    sf::Text slot_price_txt( m_font, std::to_string( price ), 30 );
    slot_price_txt.setFillColor( sf::Color::Black );

    const std::string idx_key = "slot" + std::to_string( i + 1 ) + "_idx";
    const std::string desc_key = "slot" + std::to_string( i + 1 ) + "_desc";
    const std::string price_key = "slot" + std::to_string( i + 1 ) + "_price";
    for ( const auto &ui_label : m_shop_ui_data->m_labels )
    {
      if ( ui_label.name.contains( idx_key ) ) { draw_label_text_at( slot_idx_txt, ui_label.rect.position, ui_label.align ); }
      else if ( ui_label.name.contains( desc_key ) ) { draw_label_text_at( slot_desc_txt, ui_label.rect.position, ui_label.align ); }
      else if ( ui_label.name.contains( price_key ) ) { draw_label_text_at( slot_price_txt, ui_label.rect.position, ui_label.align ); }
    }
  }
}

void RenderOverlaySystem::render_ghost_grave_halos()
{
  // auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  // auto wearlevel = Utils::Player::get_inventory_wear_level( reg() );
  // if ( inventory_type != "item.elderflute" or ( inventory_type == "item.elderflute" and wearlevel == 0 ) ) return;
  // const int player_luck = Utils::Player::get_stats( reg() ).luck();

  // for ( auto [grave_entt, grave_mb, consequence] : reg().view<Cmp::Grave::MultiBlock, Cmp::Grave::Consequence>().each() )
  // {
  //   if ( not Utils::is_visible_in_view( get_screen_view(), grave_mb ) ) continue;
  //   if ( consequence.get( player_luck ) != Cmp::Grave::Consequence::Type::NPC_TRAP ) continue;
  //   sf::RectangleShape rect( grave_mb.size );
  //   rect.setPosition( grave_mb.position );
  //   rect.setFillColor( sf::Color::Transparent );
  //   rect.setOutlineColor( sf::Color::Blue );
  //   rect.setOutlineThickness( 1.f );
  //   draw_world( rect );
  // }
}

void RenderOverlaySystem::render_crypt_maze_timer( sf::Vector2f pos, unsigned int size )
{
  const auto kCryptShuffleTimeout = Sys::PersistSystem::get<Cmp::Persist::CryptShuffleTimeout>( reg() ).get_value();

  for ( auto [timer_entt, timer_cmp] : reg().view<Cmp::Crypt::ShuffleTimer>().each() )
  {
    if ( not Utils::Crypt::is_crypt_shuffle_timer_expired( reg() ) )
    {
      sf::Text clock_text( m_font, "", size );
      std::stringstream ss;
      ss << std::fixed << std::setprecision( 1 ) << kCryptShuffleTimeout - timer_cmp.m_elapsed.asSeconds();
      clock_text.setString( ss.str() );
      clock_text.setPosition( { pos.x - ( clock_text.getLocalBounds().size.x / 2 ), pos.y } );
      clock_text.setFillColor( sf::Color::Red );
      clock_text.setOutlineColor( sf::Color::Black );
      clock_text.setOutlineThickness( 2.f );
      draw_screen( clock_text );
    }
  }
}

} // namespace Game::Sys