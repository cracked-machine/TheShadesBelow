#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Persistent/ResetTutorial.hpp>
#include <Components/Persistent/TooltipsSeen.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/TooltipSystem.hpp>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Game::Sys
{

namespace
{

//! @brief Read an sf::Color from a json object with r, g, b and (optional) a fields.
sf::Color get_colour( const nlohmann::json &json_data, const sf::Color &fallback )
{
  if ( not json_data.is_object() ) { return fallback; }
  return { json_data.value( "r", fallback.r ), json_data.value( "g", fallback.g ), json_data.value( "b", fallback.b ),
           json_data.value( "a", fallback.a ) };
}

} // namespace

TooltipSystem::TooltipSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : RenderSystem( reg, window, sound_bank )
{
  load_definitions( "res/json/tooltips.json" );
  std::ignore = get_systems_event_queue().sink<Events::AddInventoryEvent>().connect<&TooltipSystem::on_add_inventory_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::ItemPickedUpEvent>().connect<&TooltipSystem::on_item_picked_up_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::DismissTooltipEvent>().connect<&TooltipSystem::on_dismiss_tooltip_event>( this );
  SPDLOG_DEBUG( "TooltipSystem initialized with {} tooltips", m_definitions.size() );
}

void TooltipSystem::load_definitions( const std::string &path )
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
    if ( panel.contains( "position" ) and panel["position"].is_object() )
    {
      m_style.position = { panel["position"].value( "x", m_style.position.x ), panel["position"].value( "y", m_style.position.y ) };
    }
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

void TooltipSystem::on_add_inventory_event( const Events::AddInventoryEvent &event )
{
  if ( not m_enabled ) { return; }
  m_candidates.push_back( { "AddInventoryEvent", event.m_item.str() } );
}

void TooltipSystem::on_item_picked_up_event( const Events::ItemPickedUpEvent &event )
{
  if ( not m_enabled ) { return; }
  m_candidates.push_back( { "ItemPickedUpEvent", event.m_item.str() } );
}

void TooltipSystem::update( sf::Time dt )
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

      // the newest tooltip replaces any tooltip already showing
      m_active = ActiveTooltip{ def->text, def->duration };
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
}

void TooltipSystem::check_reset()
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

void TooltipSystem::sync_seen()
{
  auto &scene_seen = PersistSystem::get<Cmp::Persist::TooltipsSeen>( reg() ).get_value();
  m_seen.insert( scene_seen.begin(), scene_seen.end() );
  scene_seen = m_seen;
}

const TooltipSystem::TooltipDef *TooltipSystem::find_match( const Candidate &candidate ) const
{
  for ( const auto &def : m_definitions )
  {
    if ( def.trigger != candidate.trigger ) { continue; }
    if ( not def.item.empty() and def.item != candidate.item ) { continue; }
    if ( def.frequency == Frequency::DISABLED ) { continue; }
    if ( def.frequency == Frequency::ONCE and m_seen.contains( def.id ) ) { continue; }
    return &def;
  }
  return nullptr;
}

void TooltipSystem::render()
{
  if ( not m_active ) { return; }

  const sf::Vector2u display_size = PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );
  const sf::Vector2f centre{ static_cast<float>( display_size.x ) * m_style.position.x, static_cast<float>( display_size.y ) * m_style.position.y };

  sf::Text text( m_font, m_active->text, m_style.font_size );
  text.setFillColor( m_style.text_colour );
  const sf::FloatRect text_bounds = text.getLocalBounds();
  text.setOrigin( text_bounds.position + ( text_bounds.size * 0.5f ) );
  text.setPosition( centre );

  sf::RectangleShape panel( text_bounds.size + sf::Vector2f{ m_style.padding * 2.f, m_style.padding * 2.f } );
  panel.setOrigin( panel.getSize() * 0.5f );
  panel.setPosition( centre );
  panel.setFillColor( m_style.fill_colour );
  panel.setOutlineColor( m_style.line_colour );
  panel.setOutlineThickness( 2.f );

  draw_screen( panel );
  draw_screen( text );
}

} // namespace Game::Sys
