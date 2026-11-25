#include <Components/Altar/Activated.hpp>
#include <Components/Altar/BloodCount.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/AbsoluteRenderOffset.hpp>
#include <Components/AnimData.hpp>
#include <Components/Crypt/ShuffleTimer.hpp>
#include <Components/DamagePopup.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Health.hpp>
#include <Components/Inventory/FlashUIExtraLife.hpp>
#include <Components/Inventory/FlashUIHealth.hpp>
#include <Components/Inventory/FlashUIInventory.hpp>
#include <Components/Inventory/FlashUIWealth.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/NoRender.hpp>
#include <Components/Particle/SpriteBase.hpp>
#include <Components/Particle/SpriteOwner.hpp>
#include <Components/Persistent/AltarBloodQuota.hpp>
#include <Components/Persistent/CryptShuffleTimeout.hpp>
#include <Components/Persistent/MaxLevelDepth.hpp>
#include <Components/Persistent/ShowDamageText.hpp>
#include <Components/Persistent/ShowHealthBar.hpp>
#include <Components/Persistent/StatChangeDisplayDuration.hpp>
#include <Components/Persistent/ToxicImmunityMeterFill.hpp>
#include <Components/Persistent/ToxicImmunityMeterShimmer.hpp>
#include <Components/Persistent/ToxicImmunityMeterTint.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Player/ToxicImmunity.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Components/Toxicity/Venom.hpp>
#include <Components/Toxicity/Vertigo.hpp>
#include <Components/UUID.hpp>
#include <Factory/ParticleFactory.hpp>
#include <SFML/System/Time.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/HudOverlaySystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Crypt.hpp>
#include <Utils/ImGuiTheme.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Profiling.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <format>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace Game::Sys
{

void HudOverlaySystem::update( sf::Time dt )
{
  // the layout is authored at its Tiled map size; rescale it if the window resolution has changed (no-op otherwise)
  if ( m_main_ui_layout ) m_main_ui_layout->sync_to( m_window.getSize() );
  // one inventory slot's layout follows the HUD's scale; each slot adds its own origin when drawn (no-op if unchanged)
  if ( m_slot_ui_layout ) m_slot_ui_layout->sync_to_scale( hud_scale() );

  PROFILED( fit_inventory_outline() );
  PROFILED( render_ui_outlines() );
  PROFILED( render_ui_icons() );
  PROFILED( render_ui_inventory_table( dt ) );
  // after the inventory table, so the torch flames flicker over their slot icons
  PROFILED( render_ui_particles() );
  // the meters (with their frost) and the texts share one ImGui window, texts submitted last so they draw over the
  // meters. ImGui renders after every SFML draw, so nothing SFML can be drawn over these.
  begin_overlay_window( "##hud_meters" );
  // the HUD is the base ImGui layer: every other overlay's ImGui window (grimoire, tooltips, pause menu) draws over it,
  // whatever order the windows were created in
  ImGui::BringWindowToDisplayBack( ImGui::GetCurrentWindow() );
  // first, so the HUD draws over the altar meters
  PROFILED( render_altar_blood_meters() );
  PROFILED( render_world_health_bars() );
  PROFILED( render_damage_popups( dt ) );
  PROFILED( render_ui_meters( dt ) );
  PROFILED( render_ui_labels( dt ) );
  PROFILED( render_ui_texts() );
  const auto display_size = sf::Vector2f( m_window.getSize() );
  render_crypt_maze_timer( { display_size.x / 2.f, 0.f }, scaled_char_size( 100, m_main_ui_layout.get() ) );
  end_overlay_window();
  PROFILED( render_ghost_grave_halos() );
}

void HudOverlaySystem::render_ui_particles()
{
  if ( not m_main_ui_layout ) return;

  // only carried torches are moved into screen-space; each lit one flickers over its own inventory slot icon
  auto view = reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>( entt::exclude<Cmp::NoRender> );
  for ( auto [entity, particle_owner, particle_uuid_cmp] : view.each() )
  {
    if ( not particle_owner.sprite ) continue;
    auto &sprite = *particle_owner.sprite;
    if ( sprite.get_view_type() != Cmp::Particle::ViewType::SCREEN ) continue;
    if ( not sprite.get_tag().contains( "torch" ) ) continue;

    // the slot holding this torch, if it is lit (selected, or stowed with Cmp::Persist::StowedTorchLit) and was drawn
    std::optional<SlotIcon> slot_icon;
    for ( auto [slot_entt, slot_cmp, slot_uuid_cmp] : reg().view<Cmp::PlayerInventorySlot, Cmp::UUID>().each() )
    {
      if ( slot_uuid_cmp != particle_uuid_cmp ) continue;
      if ( slot_cmp.m_slot_idx >= m_slot_icons.size() ) continue;
      if ( Utils::Player::is_torch_slot_lit( reg(), slot_entt ) ) { slot_icon = m_slot_icons.at( slot_cmp.m_slot_idx ); }
      break;
    }
    if ( not slot_icon ) continue;

    sprite.set_view_transform( m_window, m_window.getDefaultView() );
    sprite.set_emitter_position( { slot_icon->position.x + ( slot_icon->scale * 8.f ), slot_icon->position.y + ( slot_icon->scale * 6.f ) } );
    sprite.restart();
    draw_screen( sprite );
  }
}

void HudOverlaySystem::render_ui_outlines()
{
  if ( not m_main_ui_layout )
  {
    SPDLOG_CRITICAL( "UiLayout object is not initialised. Cannot draw Status Outline overlay" );
    return;
  }
  render_outlines( *m_main_ui_layout );
}

void HudOverlaySystem::render_ui_icons()
{
  if ( not m_main_ui_layout )
  {
    SPDLOG_CRITICAL( "UiLayout object is not initialised. Cannot draw icon overlay" );
    return;
  }
  for ( const auto &icon : m_main_ui_layout->m_icons )
  {
    RenderSystem::safe_render_sprite_screen( icon.type, icon.rect, icon.index, { icon.scale, icon.scale } );
  }
}

const Render::UiLayout::Table *HudOverlaySystem::find_inventory_table() const
{
  if ( not m_main_ui_layout ) return nullptr;
  const auto it = std::ranges::find( m_main_ui_layout->m_tables, std::string( kInventoryTableName ), &Render::UiLayout::Table::name );
  return it == m_main_ui_layout->m_tables.end() ? nullptr : &*it;
}

namespace
{
//! @brief Window flags for the HUD's ImGui windows: display only, so the HUD never takes the mouse from the game or
//! comes in front of other overlays.
constexpr ImGuiWindowFlags kHudWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollbar;

//! @brief Convert an SFML colour to an ImGui colour.
ImVec4 to_imvec4( sf::Color colour )
{
  return { static_cast<float>( colour.r ) / 255.f, static_cast<float>( colour.g ) / 255.f, static_cast<float>( colour.b ) / 255.f,
           static_cast<float>( colour.a ) / 255.f };
}

//! @brief Find a layout element by name in one of a UiLayout's element lists.
template <typename ElementT>
const ElementT *find_element( const std::vector<ElementT> &elements, std::string_view name )
{
  const auto it = std::ranges::find_if( elements, [name]( const ElementT &element ) { return element.name == name; } );
  return it == elements.end() ? nullptr : &*it;
}
} // namespace

sf::Vector2f HudOverlaySystem::inventory_slot_size() const
{
  if ( not m_slot_ui_layout ) return { 0.f, 0.f };
  return m_slot_ui_layout->design_size() * hud_scale();
}

void HudOverlaySystem::fit_inventory_outline()
{
  const auto *table = find_inventory_table();
  if ( not table ) return;
  const auto it = std::ranges::find( m_main_ui_layout->m_outlines, std::string( kInventoryOutlineName ), &Render::UiLayout::Outline::name );
  if ( it == m_main_ui_layout->m_outlines.end() ) return;

  // the authored outline pads the table; keep that padding around all the slots. Recomputed from the table every
  // frame, so the size never accumulates between UiLayout::sync_to() rebuilds.
  const sf::Vector2f pad = table->rect.position - it->rect.position;
  const sf::Vector2f slot_size = inventory_slot_size();
  const float slot_count = static_cast<float>( Utils::Player::get_inventory_slot_count( reg() ) );
  it->rect.size = { ( slot_count * slot_size.x ) + ( 2.f * pad.x ), slot_size.y + ( 2.f * pad.y ) };
}

void HudOverlaySystem::render_ui_inventory_table( sf::Time dt )
{
  m_slot_icons.fill( std::nullopt );
  const auto *table = find_inventory_table();
  if ( not table or not m_slot_ui_layout )
  {
    SPDLOG_CRITICAL( "No '{}' ui_table in the HUD layout, or no {}. Cannot draw the inventory", kInventoryTableName, kInventorySlotLayout );
    return;
  }

  const uint8_t slot_count = Utils::Player::get_inventory_slot_count( reg() );
  const uint8_t selected_idx = Utils::Player::get_selected_slot_idx( reg() );
  const sf::Vector2f slot_size = inventory_slot_size();
  // flash the selected slot's highlight on a recent change (e.g. a repaired tool or an altar reward)
  const bool flash = update_flash_toggle<Cmp::FlashUIInventory>( dt );

  // the ui_table only gives the position; each column is one slot of res/ui/inventory_slot.json
  ImGui::SetNextWindowPos( ImVec2( table->rect.position.x, table->rect.position.y ) );
  ImGui::SetNextWindowSize( ImVec2( slot_size.x * static_cast<float>( slot_count ), slot_size.y ) );
  ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.f, 0.f ) );
  ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 0.f, 0.f ) );
  ImGui::PushFont( Utils::imgui_grimoire_font() );
  if ( ImGui::Begin( "##hud_inventory", nullptr, kHudWindowFlags ) )
  {
    ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.f, 1.f, 1.f, 1.f ) );
    ImGui::PushStyleColor( ImGuiCol_TableBorderLight, ImVec4( 0.f, 0.f, 0.f, 0.5f ) );
    ImGui::PushStyleColor( ImGuiCol_PlotHistogram, ImVec4( 1.f, 0.f, 0.f, 1.f ) );
    ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.f, 0.f, 0.f, 0.5f ) );
    if ( ImGui::BeginTable( "##inventory", slot_count, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit ) )
    {
      for ( uint8_t idx = 0; idx < slot_count; ++idx )
      {
        ImGui::TableSetupColumn( nullptr, ImGuiTableColumnFlags_WidthFixed, slot_size.x );
      }
      ImGui::TableNextRow();
      // column `idx` is always slot `idx`, so emptying a slot never shifts the others
      for ( uint8_t idx = 0; idx < slot_count; ++idx )
      {
        ImGui::TableSetColumnIndex( idx );
        const ImVec2 cell_min = ImGui::GetCursorScreenPos();
        render_inventory_slot( idx, idx == selected_idx, flash, { cell_min.x, cell_min.y } );
      }
      ImGui::EndTable();
    }
    ImGui::PopStyleColor( 4 );
  }
  ImGui::End();
  ImGui::PopFont();
  ImGui::PopStyleVar( 2 );
}

void HudOverlaySystem::place_imgui_text( const Render::UiLayout::Label &label, sf::Vector2f slot_origin, const std::string &text )
{
  // the font is rasterised at kImGuiGrimoireFontPx, so scale it to the label's (HUD scaled) font size
  ImGui::SetWindowFontScale( static_cast<float>( label.font_size ) / Utils::kImGuiGrimoireFontPx );
  // like the HUD's other labels: the point is the text's top, and its left edge, centre or right edge per `align`
  sf::Vector2f pos = slot_origin + label.rect.position;
  const float text_w = ImGui::CalcTextSize( text.c_str() ).x;
  if ( label.align == "center" ) { pos.x -= text_w / 2.f; }
  else if ( label.align == "right" ) { pos.x -= text_w; }
  ImGui::SetCursorScreenPos( ImVec2( pos.x, pos.y ) );
  ImGui::TextUnformatted( text.c_str() );
}

void HudOverlaySystem::render_inventory_slot( uint8_t idx, bool selected, bool flash, sf::Vector2f slot_origin )
{
  const auto &layout = *m_slot_ui_layout;
  const auto offset_rect = [slot_origin]( const sf::FloatRect &rect ) { return sf::FloatRect( slot_origin + rect.position, rect.size ); };

  // the highlight is drawn by SFML behind the icon, as ImGui draws over the SFML sprites and would tint them
  if ( const auto *highlight_cmp = find_element( layout.m_outlines, kSlotHighlightName ); selected and highlight_cmp )
  {
    const auto rect = offset_rect( highlight_cmp->rect );
    const auto thickness = static_cast<float>( highlight_cmp->line_thickness );
    // drawn inside the authored rect, so a full-size highlight doesn't spill into the next slot
    sf::RectangleShape highlight( { rect.size.x - ( 2.f * thickness ), rect.size.y - ( 2.f * thickness ) } );
    highlight.setPosition( rect.position + sf::Vector2f( thickness, thickness ) );
    highlight.setFillColor( highlight_cmp->fill_color );
    highlight.setOutlineThickness( thickness );
    highlight.setOutlineColor( flash ? sf::Color::White : highlight_cmp->line_color );
    draw_screen( highlight );
  }

  const auto slot_entt = Utils::Player::get_inventory_slot_entt( reg(), idx );
  const auto *slot_cmp = slot_entt == entt::null ? nullptr : &reg().get<Cmp::PlayerInventorySlot>( slot_entt );
  const bool has_item = slot_cmp != nullptr and not slot_cmp->empty();

  // icon, shrunk (x and y, keeping its aspect) to fit the slot_icon rect and centred in it, e.g. a 1x2 plant fits its height
  const auto *icon_cmp = find_element( layout.m_icons, kSlotIconName );
  const auto *anim_cmp = has_item ? reg().try_get<Cmp::AnimData>( slot_entt ) : nullptr;
  if ( icon_cmp and anim_cmp )
  {
    const auto box = offset_rect( icon_cmp->rect );
    const sf::Vector2f sprite_size = Sys::SpriteStore::instance().get( anim_cmp->m_sprite_type ).sprite_size();
    const float icon_scale = std::min( box.size.x / sprite_size.x, box.size.y / sprite_size.y );
    const sf::Vector2f icon_pos = box.position + ( ( box.size - ( sprite_size * icon_scale ) ) / 2.f );
    // the slot keeps its item's frame (items.json sprite.index, or the frame rolled by sprite.random) as the anim offset
    RenderSystem::safe_render_sprite_screen( anim_cmp->m_sprite_type, { icon_pos, sprite_size }, anim_cmp->getFrameIndexOffset(),
                                             { icon_scale, icon_scale } );
    m_slot_icons.at( idx ) = SlotIcon{ .position = icon_pos, .scale = icon_scale };
  }

  if ( const auto *number_cmp = find_element( layout.m_labels, kSlotNumberName ) )
  {
    place_imgui_text( *number_cmp, slot_origin, std::to_string( idx + 1 ) );
  }

  if ( const auto *name_cmp = find_element( layout.m_labels, kSlotNameName ); name_cmp and has_item )
  {
    place_imgui_text( *name_cmp, slot_origin, Sys::SpriteStore::instance().get( slot_cmp->m_item->sprite_type ).display_name() );
  }

  if ( const auto *count_cmp = find_element( layout.m_labels, kSlotCountName ); count_cmp and has_item )
  {
    if ( const int stack_count = Utils::Player::get_stack_count( reg(), slot_entt ); stack_count > 1 )
    {
      place_imgui_text( *count_cmp, slot_origin, std::format( "x{}", stack_count ) );
    }
  }

  const auto *wear_cmp = has_item ? reg().try_get<Cmp::Inventory::WearLevel>( slot_entt ) : nullptr;
  if ( const auto *meter_cmp = find_element( layout.m_meters, kSlotWearName ); meter_cmp and wear_cmp )
  {
    const auto rect = offset_rect( meter_cmp->rect );
    ImGui::SetCursorScreenPos( ImVec2( rect.position.x, rect.position.y ) );
    ImGui::ProgressBar( std::clamp( wear_cmp->m_level / 100.f, 0.f, 1.f ), ImVec2( rect.size.x, rect.size.y ), "" );
  }

  // size the cell to the whole slot, whatever was drawn: ImGui only grows a cell (and the row) from submitted items
  const sf::Vector2f slot_size = inventory_slot_size();
  ImGui::SetCursorScreenPos( ImVec2( slot_origin.x, slot_origin.y ) );
  ImGui::Dummy( ImVec2( slot_size.x, slot_size.y ) );
}

void HudOverlaySystem::render_ui_meters( sf::Time dt )
{
  if ( not m_main_ui_layout )
  {
    SPDLOG_CRITICAL( "UiLayout object is not initialised. Cannot draw icon overlay" );
    return;
  }
  // toxidrome meters get a frozen fill and a frosted overlay while the player is immune, blinking as the immunity runs out
  const auto *immunity_cmp = reg().try_get<Cmp::Player::ToxicImmunity>( Utils::Player::get_entity( reg() ) );
  const bool immunity_visible = immunity_cmp and immunity_cmp->indicator_visible();
  // advance once per frame, not per meter, so every frozen meter shares the same shader time
  if ( immunity_visible ) { m_frost_timer += dt; }
  const sf::Color toxidrome_fill = immunity_visible ? sf::Color( Sys::PersistSystem::get<Cmp::Persist::ToxicImmunityMeterFill>( reg() ) )
                                                    : sf::Color::Green;
  const sf::Color toxidrome_overlay = Sys::PersistSystem::get<Cmp::Persist::ToxicImmunityMeterTint>( reg() );
  const sf::Color toxidrome_glint = Sys::PersistSystem::get<Cmp::Persist::ToxicImmunityMeterShimmer>( reg() );
  // gives each frozen meter its own frost pattern
  int toxidrome_index = 0;
  const float kMeterOutlineThickness = 5.f * hud_scale();

  for ( const auto &meter : m_main_ui_layout->m_meters )
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

    if ( not should_render ) { continue; }

    // the frame is the meter rect plus the outline: ProgressBar insets its fill by the border size, so the fill covers the
    // authored rect and the border sits round it, like the old SFML outline
    const sf::Vector2f outline( kMeterOutlineThickness, kMeterOutlineThickness );
    const sf::FloatRect frame_rect( meter.rect.position - outline, meter.rect.size + ( outline * 2.f ) );
    ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, kMeterOutlineThickness );
    ImGui::PushStyleColor( ImGuiCol_Border, to_imvec4( meter_outer_color ) );
    ImGui::PushStyleColor( ImGuiCol_BorderShadow, ImVec4( 0.f, 0.f, 0.f, 0.f ) );
    ImGui::PushStyleColor( ImGuiCol_PlotHistogram, to_imvec4( meter_inner_color ) );
    // the meters have no background
    ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.f, 0.f, 0.f, 0.f ) );
    ImGui::SetCursorScreenPos( ImVec2( frame_rect.position.x, frame_rect.position.y ) );
    ImGui::ProgressBar( std::clamp( meter_value / 100.f, 0.f, 1.f ), ImVec2( frame_rect.size.x, frame_rect.size.y ), "" );
    ImGui::PopStyleColor( 4 );
    ImGui::PopStyleVar();

    // frost the whole toxidrome meter, outline included, so even an empty meter shows the immunity. Submitted right after
    // the meter's ProgressBar, so it draws over it.
    if ( immunity_visible and is_toxidrome_meter )
    {
      if ( load_frost_shader() )
      {
        render_meter_frost( meter.name, frame_rect, static_cast<float>( toxidrome_index ) * 17.31f, toxidrome_overlay, toxidrome_glint );
      }
      else
      {
        // no shader support: fall back to a flat frost wash
        ImGui::GetWindowDrawList()->AddRectFilled( ImVec2( frame_rect.position.x, frame_rect.position.y ),
                                                   ImVec2( frame_rect.position.x + frame_rect.size.x, frame_rect.position.y + frame_rect.size.y ),
                                                   ImGui::GetColorU32( to_imvec4( toxidrome_overlay ) ) );
      }
      ++toxidrome_index;
    }
  }
}

bool HudOverlaySystem::load_frost_shader()
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

void HudOverlaySystem::render_meter_frost( const std::string &meter_name, const sf::FloatRect &rect, float seed, sf::Color frost, sf::Color glint )
{
  // ImGui draws after every SFML draw, so the frost is rendered into the meter's own texture and shown with ImGui::Image
  // over the meter's ProgressBar. One texture per meter: ImGui only reads them when the frame is rendered.
  const sf::Vector2u size( static_cast<unsigned int>( std::max( 1.f, std::round( rect.size.x ) ) ),
                           static_cast<unsigned int>( std::max( 1.f, std::round( rect.size.y ) ) ) );
  auto &texture = m_frost_textures[meter_name];
  if ( texture.getSize() != size and not texture.resize( size ) )
  {
    SPDLOG_WARN( "Unable to create the {} frost texture", meter_name );
    return;
  }

  // the shader works in render target pixels (gl_FragCoord), which are now the texture's own
  const sf::Vector2f texture_size( size );
  m_frost_shader.setUniform( "rect", sf::Glsl::Vec4( 0.f, 0.f, texture_size.x, texture_size.y ) );
  m_frost_shader.setUniform( "target_height", texture_size.y );
  m_frost_shader.setUniform( "time", m_frost_timer.asSeconds() );
  m_frost_shader.setUniform( "seed", seed );
  m_frost_shader.setUniform( "frost", sf::Glsl::Vec4( frost ) );
  m_frost_shader.setUniform( "glint", sf::Glsl::Vec4( glint ) );

  texture.clear( sf::Color::Transparent );
  sf::RectangleShape overlay( texture_size );
  overlay.setFillColor( sf::Color::White );
  texture.draw( overlay, sf::RenderStates( &m_frost_shader ) );
  texture.display();

  ImGui::SetCursorScreenPos( ImVec2( rect.position.x, rect.position.y ) );
  ImGui::Image( texture, rect.size );
}

void HudOverlaySystem::render_ui_texts()
{
  if ( not m_main_ui_layout )
  {
    SPDLOG_CRITICAL( "UiLayout object is not initialised. Cannot draw value overlay" );
    return;
  }

  // ImGui text, in the HUD window after the meters, so the toxidrome names stay readable on top of their bars
  const float outline = 2.f * hud_scale();
  for ( const auto &ui_text : m_main_ui_layout->m_texts )
  {
    draw_text( ui_text.value, static_cast<float>( ui_text.font_size ), ui_text.rect.position, sf::Color::White, TextAlign::LEFT, sf::Color::Black,
               outline );
  }
}

//! @brief Advances `interval` by `dt` while a `FlashComponent` is present on any entity, removing it once its duration has elapsed.
//! Returns whether the UI element should currently be drawn in its "flashed" state (toggles on/off at m_ui_flash_factor ms).
//! @tparam FlashComponent
//! @param dt
//! @param interval accumulated flash time; reset to zero once the flash expires
//! @return true if the UI element should currently be drawn in its "flashed" state, false otherwise.
template <typename FlashComponent>
bool HudOverlaySystem::update_flash_toggle( sf::Time dt )
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

void HudOverlaySystem::render_ui_labels( sf::Time dt )
{
  if ( not m_main_ui_layout )
  {
    SPDLOG_CRITICAL( "UiLayout object is not initialised. Cannot draw value overlay" );
    return;
  }
  for ( const auto &ui_label : m_main_ui_layout->m_labels )
  {
    std::string text_str;
    if ( ui_label.name == "wealth_label" ) { text_str = " =   " + std::to_string( Utils::Player::get_wealth( reg() ).wealth ); }
    else if ( ui_label.name.ends_with( "_delta_lbl" ) )
    {
      text_str = update_stat_change_text( ui_label.name, dt );
      if ( text_str.empty() ) { continue; }
    }

    sf::Color outline = sf::Color::Black;
    // flash the text if we just deposited something in a well
    if ( ui_label.name == "wealth_label" and update_flash_toggle<Cmp::FlashUIWealth>( dt ) ) { outline = sf::Color::White; }

    draw_text( text_str, static_cast<float>( ui_label.font_size ), ui_label.rect.position, sf::Color::White, to_text_align( ui_label.align ), outline,
               2.f * hud_scale() );
  }
}

std::optional<int> HudOverlaySystem::get_change_label_stat( const std::string &label_name )
{
  const auto &stats = Utils::Player::get_stats( reg() );
  const auto &toxidrome = stats.toxidrome();
  if ( label_name == "health_delta_lbl" ) { return stats.health(); }
  if ( label_name == "fear_delta_lbl" ) { return stats.fear(); }
  if ( label_name == "infamy_delta_lbl" ) { return stats.infamy(); }
  if ( label_name == "despair_delta_lbl" ) { return stats.despair(); }
  if ( label_name == "luck_delta_lbl" ) { return stats.luck(); }
  if ( label_name == "tachycardia_delta_lbl" ) { return toxidrome.at<Cmp::Toxicity::Tachycardia>().value_or( 0 ); }
  if ( label_name == "bradycardia_delta_lbl" ) { return toxidrome.at<Cmp::Toxicity::Bradycardia>().value_or( 0 ); }
  if ( label_name == "hallucinogen_delta_lbl" ) { return toxidrome.at<Cmp::Toxicity::Hallucinogen>().value_or( 0 ); }
  if ( label_name == "hypoxia_delta_lbl" ) { return toxidrome.at<Cmp::Toxicity::Hypoxia>().value_or( 0 ); }
  if ( label_name == "phototoxia_delta_lbl" ) { return toxidrome.at<Cmp::Toxicity::Phototoxia>().value_or( 0 ); }
  if ( label_name == "venom_delta_lbl" ) { return toxidrome.at<Cmp::Toxicity::Venom>().value_or( 0 ); }
  if ( label_name == "vertigo_delta_lbl" ) { return toxidrome.at<Cmp::Toxicity::Vertigo>().value_or( 0 ); }
  return std::nullopt;
}

std::string HudOverlaySystem::update_stat_change_text( const std::string &label_name, sf::Time dt )
{
  const auto current_value = get_change_label_stat( label_name );
  if ( not current_value )
  {
    SPDLOG_WARN( "No player stat matches UI label '{}'", label_name );
    return "";
  }

  // a new player (e.g. a new game) starts a fresh baseline rather than showing the reset as a change
  const auto player_entity = Utils::Player::get_entity( reg() );
  if ( player_entity != m_stat_changes_player )
  {
    m_stat_changes.clear();
    m_stat_changes_player = player_entity;
  }

  auto [it, inserted] = m_stat_changes.try_emplace( label_name, StatChange{ .last_value = *current_value } );
  auto &change = it->second;
  if ( inserted ) { return ""; }

  const auto kDisplayDuration = sf::seconds( Sys::PersistSystem::get<Cmp::Persist::StatChangeDisplayDuration>( reg() ).get_value() );
  if ( *current_value != change.last_value )
  {
    // restart the display, accumulating into a delta that is still showing
    if ( change.elapsed >= kDisplayDuration ) { change.delta = 0; }
    change.delta += *current_value - change.last_value;
    change.last_value = *current_value;
    change.elapsed = sf::Time::Zero;
  }
  else if ( change.elapsed < kDisplayDuration ) { change.elapsed += dt; }

  if ( change.elapsed >= kDisplayDuration or change.delta == 0 ) { return ""; }
  return std::format( "{:+}", change.delta );
}

void HudOverlaySystem::render_level_depth()
{
  const Cmp::Player::LevelDepth &level_depth_cmp = Utils::Player::get_level_depth( reg() );
  if ( level_depth_cmp.display_timer >= level_depth_cmp.display_cooldown ) { return; }

  const int max_level_depth = Sys::PersistSystem::get<Cmp::Persist::MaxLevelDepth>( reg() ).get_value();
  const auto display_res = sf::Vector2f( m_window.getSize() );
  // its own window: this is drawn after update(), outside the HUD's other windows
  begin_overlay_window( "##hud_level_depth" );
  // behind every other overlay, like the HUD's meters
  ImGui::BringWindowToDisplayBack( ImGui::GetCurrentWindow() );
  draw_text( "Nekropolis " + std::to_string( max_level_depth - level_depth_cmp.get_count() ),
             static_cast<float>( scaled_char_size( 100, m_main_ui_layout.get() ) ), { display_res.x / 2.f, 100.f * hud_scale() }, sf::Color::White,
             TextAlign::CENTER );
  end_overlay_window();
}

void HudOverlaySystem::render_altar_blood_meters()
{
  // the meter's size in world pixels, centred on the altar
  constexpr sf::Vector2f kMeterWorldSize{ 6.f, 28.f };
  constexpr float kMeterWorldOutline = 1.f;
  const sf::Color kMeterFill( 200, 16, 16, 255 );
  const sf::Color kMeterBackground( 0, 0, 0, 160 );
  const sf::Color kMeterOutline( 64, 64, 64, 255 );

  const int quota = Sys::PersistSystem::get<Cmp::Persist::AltarBloodQuota>( reg() ).get_value();
  const sf::View &world_view = Sys::RenderSystem::get_world_view();
  // ImGui draws in window pixels
  const float px_per_world_unit = static_cast<float>( m_window.getSize().y ) / world_view.getSize().y;
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  for ( auto [altar_entt, altar_cmp, blood_cmp] : reg().view<Cmp::Altar::MultiBlock, Cmp::Altar::BloodCount>().each() )
  {
    if ( altar_cmp.get_kind() != Cmp::Altar::Kind::GRAVEYARD ) continue;
    if ( reg().all_of<Cmp::Altar::Activated>( altar_entt ) ) continue;
    if ( not Utils::is_visible_in_view( world_view, altar_cmp ) ) continue;

    const sf::Vector2f world_top_left = altar_cmp.getCenter() - ( kMeterWorldSize / 2.f );
    const sf::Vector2f top_left( m_window.mapCoordsToPixel( world_top_left, world_view ) );
    const sf::Vector2f bottom_right( m_window.mapCoordsToPixel( world_top_left + kMeterWorldSize, world_view ) );
    const float fill_top = bottom_right.y - ( ( bottom_right.y - top_left.y ) * blood_cmp.fraction( quota ) );

    draw_list->AddRectFilled( ImVec2( top_left.x, top_left.y ), ImVec2( bottom_right.x, bottom_right.y ),
                              ImGui::GetColorU32( to_imvec4( kMeterBackground ) ) );
    if ( fill_top < bottom_right.y )
    {
      draw_list->AddRectFilled( ImVec2( top_left.x, fill_top ), ImVec2( bottom_right.x, bottom_right.y ),
                                ImGui::GetColorU32( to_imvec4( kMeterFill ) ) );
    }
    draw_list->AddRect( ImVec2( top_left.x, top_left.y ), ImVec2( bottom_right.x, bottom_right.y ), ImGui::GetColorU32( to_imvec4( kMeterOutline ) ),
                        0.f, 0, kMeterWorldOutline * px_per_world_unit );
  }
}

void HudOverlaySystem::render_world_health_bars()
{
  if ( not Sys::PersistSystem::get<Cmp::Persist::ShowHealthBar>( reg() ).get_value() ) return;

  // the bar's inset and height in world pixels
  constexpr float kWorldPadding = 1.f;
  constexpr float kWorldHeight = 2.f;
  const ImVec4 kFill = to_imvec4( sf::Color( 255, 0, 0, 224 ) );
  const ImVec4 kBackground = to_imvec4( sf::Color( 0, 0, 0, 128 ) );

  const sf::View &world_view = Sys::RenderSystem::get_world_view();
  ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0.f );
  ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 0.f );
  ImGui::PushStyleColor( ImGuiCol_PlotHistogram, kFill );
  ImGui::PushStyleColor( ImGuiCol_FrameBg, kBackground );
  for ( auto [entity, health_cmp] : reg().view<Cmp::Health>().each() )
  {
    // full health shows nothing, so undamaged plants and NPCs don't clutter the screen
    if ( health_cmp.m_level >= 100.f ) continue;

    // a plant spans its multiblock; an NPC its position, lifted by its render offset like its sprite
    sf::FloatRect bounds;
    if ( const auto *plant_mb_cmp = reg().try_get<Cmp::PlantMultiBlock>( entity ) ) { bounds = *plant_mb_cmp; }
    else if ( const auto *pos_cmp = reg().try_get<Cmp::Position>( entity ) )
    {
      bounds = *pos_cmp;
      if ( const auto *render_offset_cmp = reg().try_get<Cmp::AbsoluteRenderOffset>( entity ) ) bounds.position += render_offset_cmp->getOffset();
    }
    else { continue; }
    if ( not Utils::is_visible_in_view( world_view, bounds ) ) continue;

    const sf::Vector2f world_top_left( bounds.position.x + kWorldPadding, bounds.position.y + kWorldPadding );
    const sf::Vector2f world_size( bounds.size.x - ( kWorldPadding * 2.f ), kWorldHeight );
    const sf::Vector2f top_left( m_window.mapCoordsToPixel( world_top_left, world_view ) );
    const sf::Vector2f bottom_right( m_window.mapCoordsToPixel( world_top_left + world_size, world_view ) );

    ImGui::PushID( static_cast<int>( entity ) );
    ImGui::SetCursorScreenPos( ImVec2( top_left.x, top_left.y ) );
    ImGui::ProgressBar( std::clamp( health_cmp.m_level / 100.f, 0.f, 1.f ), ImVec2( bottom_right.x - top_left.x, bottom_right.y - top_left.y ), "" );
    ImGui::PopID();
  }
  ImGui::PopStyleColor( 2 );
  ImGui::PopStyleVar( 2 );
}

void HudOverlaySystem::render_damage_popups( sf::Time dt )
{
  // how long a number shows, how far it rises (world px) and its size (world px)
  constexpr float kLifetimeSeconds = 0.8f;
  constexpr float kWorldRise = 8.f;
  constexpr float kWorldFontSize = 8.f;

  const bool show = Sys::PersistSystem::get<Cmp::Persist::ShowDamageText>( reg() ).get_value();
  const sf::View &world_view = Sys::RenderSystem::get_world_view();
  // ImGui draws in window pixels
  const float px_per_world_unit = static_cast<float>( m_window.getSize().y ) / world_view.getSize().y;

  // age every popup even while hidden, so turning the setting back on doesn't show stale numbers. Destroyed after the loop
  std::vector<entt::entity> expired;
  for ( auto [entity, popup_cmp] : reg().view<Cmp::DamagePopup>().each() )
  {
    popup_cmp.elapsed += dt;
    const float progress = popup_cmp.elapsed.asSeconds() / kLifetimeSeconds;
    if ( progress >= 1.f )
    {
      expired.push_back( entity );
      continue;
    }
    if ( not show ) continue;

    const sf::Vector2f world_pos( popup_cmp.world_pos.x, popup_cmp.world_pos.y - ( kWorldRise * progress ) );
    const sf::Vector2f screen_pos( m_window.mapCoordsToPixel( world_pos, world_view ) );
    const auto alpha = static_cast<std::uint8_t>( 255.f * ( 1.f - progress ) );
    draw_text( std::format( "-{}", static_cast<int>( std::round( popup_cmp.amount ) ) ), kWorldFontSize * px_per_world_unit, screen_pos,
               sf::Color( 255, 64, 64, alpha ), TextAlign::CENTER, sf::Color( 0, 0, 0, alpha ), px_per_world_unit );
  }
  for ( auto entity : expired )
  {
    reg().destroy( entity );
  }
}

void HudOverlaySystem::render_ghost_grave_halos()
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

void HudOverlaySystem::render_crypt_maze_timer( sf::Vector2f pos, unsigned int size )
{
  const auto kCryptShuffleTimeout = Sys::PersistSystem::get<Cmp::Persist::CryptShuffleTimeout>( reg() ).get_value();

  for ( auto [timer_entt, timer_cmp] : reg().view<Cmp::Crypt::ShuffleTimer>().each() )
  {
    if ( not Utils::Crypt::is_crypt_shuffle_timer_expired( reg() ) )
    {
      std::stringstream ss;
      ss << std::fixed << std::setprecision( 1 ) << kCryptShuffleTimeout - timer_cmp.m_elapsed.asSeconds();
      draw_text( ss.str(), static_cast<float>( size ), pos, sf::Color::Red, TextAlign::CENTER, sf::Color::Black, 2.f * hud_scale() );
    }
  }
}

} // namespace Game::Sys