#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Persistent/ResetTutorial.hpp>
#include <Components/Persistent/TooltipsSeen.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/TooltipOverlaySystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>

#include <imgui.h>

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string_view>

namespace Game::Sys
{

namespace
{

//! @brief Whether a tooltip's item condition matches the event's item. A condition ending in '*' matches any item key
//! with that prefix (e.g. "item.grave.blood*"), otherwise the keys must be equal. An empty condition matches any item.
bool item_condition_matches( std::string_view condition, std::string_view item )
{
  if ( condition.empty() ) return true;
  if ( condition.ends_with( '*' ) ) return item.starts_with( condition.substr( 0, condition.size() - 1 ) );
  return condition == item;
}

//! @brief Read an sf::Color from a json object with r, g, b and (optional) a fields.
sf::Color get_colour( const nlohmann::json &json_data, const sf::Color &fallback )
{
  if ( not json_data.is_object() ) { return fallback; }
  return { json_data.value( "r", fallback.r ), json_data.value( "g", fallback.g ), json_data.value( "b", fallback.b ),
           json_data.value( "a", fallback.a ) };
}

} // namespace

TooltipOverlaySystem::TooltipOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseOverlaySystem( reg, window, sound_bank )
{
  load_definitions( "res/json/tooltips.json" );
  std::ignore = get_systems_event_queue().sink<Events::AddInventoryEvent>().connect<&TooltipOverlaySystem::on_add_inventory_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::ItemPickedUpEvent>().connect<&TooltipOverlaySystem::on_item_picked_up_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::GrimoireDiscoveryEvent>().connect<&TooltipOverlaySystem::on_grimoire_discovery_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::BloodRefusedEvent>().connect<&TooltipOverlaySystem::on_blood_refused_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::AltarApproachedEvent>().connect<&TooltipOverlaySystem::on_altar_approached_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::AltarBloodOfferedEvent>().connect<&TooltipOverlaySystem::on_altar_blood_offered_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::ConsumeRefusedEvent>().connect<&TooltipOverlaySystem::on_consume_refused_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::CryptEntryRefusedEvent>().connect<&TooltipOverlaySystem::on_crypt_entry_refused_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::DismissTooltipEvent>().connect<&TooltipOverlaySystem::on_dismiss_tooltip_event>( this );
  SPDLOG_DEBUG( "TooltipOverlaySystem initialized with {} tooltips", m_definitions.size() );
}

void TooltipOverlaySystem::load_definitions( const std::string &path )
{
  std::ifstream input_file( path );
  if ( not input_file.is_open() )
  {
    SPDLOG_ERROR( "Failed to open {}, tooltips are disabled", path );
    m_enabled = false;
    return;
  }

  nlohmann::json json_data = nlohmann::json::parse( input_file, nullptr, false );
  if ( json_data.is_discarded() or not json_data.is_object() )
  {
    SPDLOG_ERROR( "Failed to parse {}, tooltips are disabled", path );
    m_enabled = false;
    return;
  }

  m_enabled = json_data.value( "enabled", true );

  if ( json_data.contains( "panel" ) and json_data["panel"].is_object() )
  {
    const auto &panel = json_data["panel"];
    m_style.font_size = panel.value( "font_size", m_style.font_size );
    m_style.padding = panel.value( "padding", m_style.padding );
    if ( panel.contains( "fill_colour" ) ) { m_style.fill_colour = get_colour( panel["fill_colour"], m_style.fill_colour ); }
    if ( panel.contains( "line_colour" ) ) { m_style.line_colour = get_colour( panel["line_colour"], m_style.line_colour ); }
    if ( panel.contains( "text_colour" ) ) { m_style.text_colour = get_colour( panel["text_colour"], m_style.text_colour ); }
  }

  if ( not json_data.contains( "tooltips" ) or not json_data["tooltips"].is_array() ) { return; }
  for ( const auto &entry : json_data["tooltips"] )
  {
    if ( not entry.is_object() or not entry.contains( "id" ) or not entry.contains( "trigger" ) or not entry.contains( "text" ) )
    {
      SPDLOG_WARN( "Skipping tooltip in {}: 'id', 'trigger' and 'text' are required", path );
      continue;
    }

    TooltipDef def;
    def.id = entry.value( "id", "" );
    def.trigger = entry.value( "trigger", "" );
    def.text = entry.value( "text", "" );
    def.duration = sf::seconds( entry.value( "duration_s", def.duration.asSeconds() ) );
    if ( entry.contains( "condition" ) and entry["condition"].is_object() ) { def.item = entry["condition"].value( "item", "" ); }
    const auto position = parse_position( entry );
    if ( not position )
    {
      SPDLOG_WARN( "Skipping tooltip '{}': 'position' must be {{\"x\", \"y\"}} or \"above_source\"", def.id );
      continue;
    }
    def.position = *position;

    const std::string frequency = entry.value( "frequency", "once" );
    if ( frequency == "once" ) { def.frequency = Frequency::ONCE; }
    else if ( frequency == "always" ) { def.frequency = Frequency::ALWAYS; }
    else if ( frequency == "disabled" ) { def.frequency = Frequency::DISABLED; }
    else
    {
      SPDLOG_WARN( "Skipping tooltip '{}': unknown frequency '{}'", def.id, frequency );
      continue;
    }

    m_definitions.push_back( std::move( def ) );
  }
}

std::optional<TooltipOverlaySystem::TooltipPosition> TooltipOverlaySystem::parse_position( const nlohmann::json &entry )
{
  TooltipPosition position;
  if ( not entry.contains( "position" ) ) { return position; }
  const auto &json_position = entry["position"];
  if ( json_position.is_string() and json_position.get<std::string>() == "above_source" )
  {
    position.above_source = true;
    return position;
  }
  if ( json_position.is_object() )
  {
    position.screen = { json_position.value( "x", position.screen.x ), json_position.value( "y", position.screen.y ) };
    return position;
  }
  return std::nullopt;
}

void TooltipOverlaySystem::on_add_inventory_event( const Events::AddInventoryEvent &event )
{
  if ( not m_enabled ) { return; }
  m_candidates.push_back( { "AddInventoryEvent", event.m_item.str() } );
}

void TooltipOverlaySystem::on_item_picked_up_event( const Events::ItemPickedUpEvent &event )
{
  if ( not m_enabled ) { return; }
  m_candidates.push_back( { "ItemPickedUpEvent", event.m_item.str() } );
}

void TooltipOverlaySystem::on_grimoire_discovery_event( const Events::GrimoireDiscoveryEvent &event )
{
  if ( not m_enabled ) { return; }
  m_candidates.push_back( { "GrimoireDiscoveryEvent", event.m_key } );
}

void TooltipOverlaySystem::on_blood_refused_event( const Events::BloodRefusedEvent &event )
{
  if ( not m_enabled ) { return; }
  m_candidates.push_back( { "BloodRefusedEvent", "", event.altar_bounds } );
}

void TooltipOverlaySystem::on_altar_approached_event( const Events::AltarApproachedEvent &event )
{
  if ( not m_enabled ) { return; }
  m_candidates.push_back( { "AltarApproachedEvent", "", event.altar_bounds } );
}

void TooltipOverlaySystem::on_altar_blood_offered_event( const Events::AltarBloodOfferedEvent &event )
{
  if ( not m_enabled ) { return; }
  m_candidates.push_back( { "AltarBloodOfferedEvent", "", event.altar_bounds } );
}

void TooltipOverlaySystem::on_consume_refused_event( const Events::ConsumeRefusedEvent &event )
{
  if ( not m_enabled ) { return; }
  m_candidates.push_back( { "ConsumeRefusedEvent", event.m_item.str() } );
}

void TooltipOverlaySystem::on_crypt_entry_refused_event( [[maybe_unused]] const Events::CryptEntryRefusedEvent &event )
{
  if ( not m_enabled ) { return; }
  m_candidates.push_back( { "CryptEntryRefusedEvent", "" } );
}

void TooltipOverlaySystem::update( sf::Time dt )
{
  check_reset();

  if ( m_enabled and not m_candidates.empty() )
  {
    sync_seen();
    bool seen_changed = false;
    for ( const auto &candidate : m_candidates )
    {
      const auto *def = find_match( candidate );
      if ( not def ) { continue; }

      // a repeating notification must not hide a one-time tutorial; otherwise the newest tooltip replaces the active one
      if ( def->frequency == Frequency::ALWAYS and m_active and m_active->frequency == Frequency::ONCE ) { continue; }
      m_active = ActiveTooltip{ format_text( *def, candidate ), def->duration, def->frequency, def->position, candidate.source_bounds };
      if ( def->frequency == Frequency::ONCE )
      {
        m_seen.insert( def->id );
        PersistSystem::get<Cmp::Persist::TooltipsSeen>( reg() ).insert( def->id );
        seen_changed = true;
      }
    }
    if ( seen_changed ) { PersistSystem::save_component<Cmp::Persist::TooltipsSeen>( reg() ); }
  }
  m_candidates.clear();

  if ( m_active )
  {
    m_active->remaining -= dt;
    if ( m_active->remaining <= sf::Time::Zero ) { m_active.reset(); }
  }

  render();
}

void TooltipOverlaySystem::check_reset()
{
  auto &reset = PersistSystem::get<Cmp::Persist::ResetTutorial>( reg() );
  if ( not reset.get_value() ) { return; }

  SPDLOG_INFO( "Resetting tutorial tooltips" );
  // m_seen must be cleared too, otherwise sync_seen() would merge the old ids straight back
  m_seen.clear();
  m_active.reset();
  PersistSystem::get<Cmp::Persist::TooltipsSeen>( reg() ).get_value().clear();
  reset.get_value() = false;
  PersistSystem::save_component<Cmp::Persist::TooltipsSeen>( reg() );
  PersistSystem::save_component<Cmp::Persist::ResetTutorial>( reg() );
}

void TooltipOverlaySystem::sync_seen()
{
  auto &scene_seen = PersistSystem::get<Cmp::Persist::TooltipsSeen>( reg() ).get_value();
  m_seen.insert( scene_seen.begin(), scene_seen.end() );
  scene_seen = m_seen;
}

const TooltipOverlaySystem::TooltipDef *TooltipOverlaySystem::find_match( const Candidate &candidate ) const
{
  for ( const auto &def : m_definitions )
  {
    if ( def.trigger != candidate.trigger ) { continue; }
    if ( not item_condition_matches( def.item, candidate.item ) ) { continue; }
    if ( def.frequency == Frequency::DISABLED ) { continue; }
    if ( def.frequency == Frequency::ONCE and m_seen.contains( def.id ) ) { continue; }
    return &def;
  }
  return nullptr;
}

std::string TooltipOverlaySystem::format_text( const TooltipDef &def, const Candidate &candidate )
{
  static constexpr std::string_view kItemPlaceholder = "{item}";
  std::string text = def.text;
  const auto pos = text.find( kItemPlaceholder );
  if ( pos == std::string::npos or candidate.item.empty() ) { return text; }

  // the candidate is an item or, for grimoire NPC pages, an NPC
  const Sys::SpriteKey sprite_key = candidate.item.starts_with( "npc." )
                                        ? Sys::NpcStore::instance().get( Sys::NpcKey( candidate.item ) ).sprite_type_list.front()
                                        : Sys::ItemStore::instance().get( Sys::ItemKey( candidate.item ) ).sprite_type;
  text.replace( pos, kItemPlaceholder.size(), Sys::SpriteStore::instance().get( sprite_key ).display_name() );
  return text;
}

sf::Vector2f TooltipOverlaySystem::panel_centre( sf::Vector2f panel_size, sf::Vector2f display_size ) const
{
  const TooltipPosition &position = m_active->position;
  if ( not position.above_source or not m_active->source_bounds )
  {
    return { display_size.x * position.screen.x, display_size.y * position.screen.y };
  }

  // centred above the source's top edge, recomputed every frame so the panel follows the camera
  constexpr float kSourceGapPx = 8.f;
  const sf::FloatRect &bounds = *m_active->source_bounds;
  const sf::Vector2f top_centre = world_to_screen( { bounds.position.x + ( bounds.size.x * 0.5f ), bounds.position.y } );
  sf::Vector2f centre{ top_centre.x, top_centre.y - kSourceGapPx - ( panel_size.y * 0.5f ) };
  // keep the whole panel on screen, e.g. when the source is near the view edge
  const sf::Vector2f half = panel_size * 0.5f;
  centre.x = std::clamp( centre.x, half.x, std::max( half.x, display_size.x - half.x ) );
  centre.y = std::clamp( centre.y, half.y, std::max( half.y, display_size.y - half.y ) );
  return centre;
}

void TooltipOverlaySystem::render()
{
  if ( not m_active ) { return; }

  const sf::Vector2u display_resolution = PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );
  const sf::Vector2f display_size( display_resolution );

  // all ImGui, panel included: ImGui renders after every SFML draw, so the tooltip stays over the HUD's ImGui widgets
  const auto font_size = static_cast<float>( m_style.font_size );
  const sf::Vector2f text_size = measure_text( m_active->text, font_size );
  const sf::Vector2f panel_size = text_size + sf::Vector2f{ m_style.padding * 2.f, m_style.padding * 2.f };
  const sf::Vector2f centre = panel_centre( panel_size, display_size );
  const sf::Vector2f panel_min = centre - ( panel_size * 0.5f );
  const sf::Vector2f panel_max = centre + ( panel_size * 0.5f );
  constexpr float kPanelOutlineThickness = 2.f;

  begin_overlay_window( "##tooltip" );
  auto &draw_list = *ImGui::GetWindowDrawList();
  const auto to_imu32 = []( sf::Color colour ) { return IM_COL32( colour.r, colour.g, colour.b, colour.a ); };
  draw_list.AddRectFilled( ImVec2( panel_min.x, panel_min.y ), ImVec2( panel_max.x, panel_max.y ), to_imu32( m_style.fill_colour ) );
  // the SFML outline was drawn outside the panel, so grow the stroke outwards by half its thickness
  const float half = kPanelOutlineThickness * 0.5f;
  draw_list.AddRect( ImVec2( panel_min.x - half, panel_min.y - half ), ImVec2( panel_max.x + half, panel_max.y + half ),
                     to_imu32( m_style.line_colour ), 0.f, 0, kPanelOutlineThickness );
  draw_text( m_active->text, font_size, { centre.x, centre.y - ( text_size.y * 0.5f ) }, m_style.text_colour, TextAlign::CENTER );
  end_overlay_window();
}

} // namespace Game::Sys
