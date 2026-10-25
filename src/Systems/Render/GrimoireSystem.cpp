#include <Audio/SoundBank.hpp>
#include <Components/Inventory/Grimoire.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Events/ItemPickedUpEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <Systems/Render/GrimoireSystem.hpp>

#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>
#include <format>
#include <optional>
#include <spdlog/spdlog.h>
#include <string_view>
#include <utility>
#include <vector>

namespace Game::Sys
{

namespace
{
constexpr float kToggleDebounceThreshold = 1.f;
constexpr float kNavigateDebounceThreshold = 0.5f;

//! @brief Map a grimoire.json object name (e.g. "forage_carry_action") to the page's subpage for its
//! prefix and the remainder of the name (e.g. "carry_action").
//! @return The subpage, or nullptr if the name has no subpage prefix.
std::pair<const Cmp::Grimoire::SubPage *, std::string_view> resolve_subpage( const Cmp::Grimoire::Page &page, std::string_view name )
{
  // the *_special_ prefixes must come before forage_/drop_ as the first matching prefix wins
  const std::pair<std::string_view, const Cmp::Grimoire::SubPage *> prefixes[] = { { "forage_special_", &page.forage_special_subpage },
                                                                                   { "drop_special_", &page.drop_special_subpage },
                                                                                   { "plant_", &page.plant_subpage },
                                                                                   { "forage_", &page.forage_subpage },
                                                                                   { "drop_", &page.drop_subpage } };
  for ( const auto &[prefix, sub_page] : prefixes )
  {
    if ( name.starts_with( prefix ) ) { return { sub_page, name.substr( prefix.size() ) }; }
  }
  return { nullptr, name };
}
} // namespace

GrimoireSystem::GrimoireSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : RenderSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&GrimoireSystem::on_player_action>( this );
  std::ignore = get_systems_event_queue().sink<Events::ItemPickedUpEvent>().connect<&GrimoireSystem::on_item_pickup_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::GrimoireUpdateEvent>().connect<&GrimoireSystem::on_grimoire_update_event>( this );

  m_ui_data = std::make_unique<Render::UiData>( "res/ui/grimoire.json" );
}

void GrimoireSystem::update( sf::Time dt )
{
  m_toggle_debounce += dt;
  m_navigate_debounce += dt;
}

bool GrimoireSystem::is_grimoire_overlay_open() const
{
  auto grimoire_view = reg().view<const Cmp::Grimoire>();
  if ( grimoire_view.empty() ) { return false; }
  return grimoire_view.get<const Cmp::Grimoire>( grimoire_view.front() ).is_enabled;
}

Cmp::Grimoire *GrimoireSystem::find_grimoire()
{
  auto grimoire_view = reg().view<Cmp::Grimoire>();
  if ( grimoire_view.empty() ) { return nullptr; }
  return &grimoire_view.get<Cmp::Grimoire>( grimoire_view.front() );
}

void GrimoireSystem::for_each_subpage( const Sys::ItemKey &item_key, const std::function<void( std::size_t, Cmp::Grimoire::SubPage & )> &fn )
{
  auto *grimoire_cmp = find_grimoire();
  if ( not grimoire_cmp or item_key.empty() ) { return; }

  for ( std::size_t page_idx = 0; page_idx < grimoire_cmp->pages.size(); ++page_idx )
  {
    Cmp::Grimoire::Page &page = grimoire_cmp->pages[page_idx];
    for ( Cmp::Grimoire::SubPage *sub_page :
          { &page.plant_subpage, &page.forage_subpage, &page.drop_subpage, &page.forage_special_subpage, &page.drop_special_subpage } )
    {
      if ( sub_page->item_key == item_key ) { fn( page_idx, *sub_page ); }
    }
  }
}

void GrimoireSystem::on_item_pickup_event( const Events::ItemPickedUpEvent &ev )
{
  const auto &item = Sys::ItemStore::instance().get( ev.m_item );
  for_each_subpage( ev.m_item, [&]( std::size_t page_idx, Cmp::Grimoire::SubPage &sub_page )
  {
    sub_page.sprite_key = item.sprite_type;
    if ( item.has_action<Cmp::CarryAction>() )
    {
      sub_page.action_map.insert_or_assign( std::string( Cmp::CarryAction::kKey ), item.get_action<Cmp::CarryAction>() );
    }
    if ( not is_grimoire_overlay_open() ) { m_current_page_idx = page_idx; }
  } );
}

void GrimoireSystem::on_grimoire_update_event( const Events::GrimoireUpdateEvent &ev )
{
  if ( ev.m_action_key == Cmp::ProximityAction::kKey and ev.m_action.is_zero() ) { return; }
  const auto &item = Sys::ItemStore::instance().get( ev.m_item );
  for_each_subpage( ev.m_item, [&]( std::size_t page_idx, Cmp::Grimoire::SubPage &sub_page )
  {
    sub_page.sprite_key = item.sprite_type;
    sub_page.action_map.insert_or_assign( ev.m_action_key, ev.m_action );
    if ( not is_grimoire_overlay_open() ) { m_current_page_idx = page_idx; }
  } );
}

void GrimoireSystem::on_player_action( Events::PlayerActionEvent ev )
{
  auto *grimoire_cmp = find_grimoire();
  if ( not grimoire_cmp ) { return; }

  if ( ev.action == Events::PlayerActionEvent::GameActions::GRIMOIRE_TOGGLE and m_toggle_debounce.asSeconds() >= kToggleDebounceThreshold )
  {
    grimoire_cmp->is_enabled = not grimoire_cmp->is_enabled;
    m_toggle_debounce = sf::Time::Zero;
    return;
  }

  if ( not grimoire_cmp->is_enabled or grimoire_cmp->pages.empty() ) { return; }
  if ( m_navigate_debounce.asSeconds() < kNavigateDebounceThreshold ) { return; }

  if ( ev.action == Events::PlayerActionEvent::GameActions::GRIMOIRE_PREV )
  {
    // step backwards and wrap around
    m_current_page_idx = ( m_current_page_idx - 1 + grimoire_cmp->pages.size() ) % grimoire_cmp->pages.size();
    m_navigate_debounce = sf::Time::Zero;
    m_sound_bank.get_effect( "page_turn" ).play();
  }
  else if ( ev.action == Events::PlayerActionEvent::GameActions::GRIMOIRE_NEXT )
  {
    // step forwards and wrap around
    m_current_page_idx = ( m_current_page_idx + 1 ) % grimoire_cmp->pages.size();
    m_navigate_debounce = sf::Time::Zero;
    m_sound_bank.get_effect( "page_turn" ).play();
  }
}

void GrimoireSystem::render()
{
  // only draw the grimoire if its enabled
  const auto *grimoire_cmp = find_grimoire();
  if ( not grimoire_cmp or not grimoire_cmp->is_enabled or grimoire_cmp->pages.empty() ) return;

  const Cmp::Grimoire::Page &current_page = grimoire_cmp->pages.at( m_current_page_idx );

  for ( const auto &outline : m_ui_data->m_outlines )
  {
    auto rect = sf::RectangleShape( outline.rect.size );
    rect.setPosition( outline.rect.position );
    rect.setFillColor( outline.fill_color );
    rect.setOutlineColor( outline.line_color );
    rect.setOutlineThickness( static_cast<float>( outline.line_thickness ) );
    draw_screen( rect );
  }
  for ( const auto &icon : m_ui_data->m_icons )
  {
    const auto [sub_page, suffix] = resolve_subpage( current_page, icon.name );
    if ( not sub_page or suffix != "icon" ) { continue; }

    Sys::SpriteKey sprite_key = sub_page->sprite_key;
    RenderSystem::safe_render_sprite_screen( sprite_key, { icon.rect.position, Constants::kGridSizePxF }, 0,
                                             { static_cast<float>( icon.scale ), static_cast<float>( icon.scale ) } );
  }
  // Tiled point y is the text baseline; sf::Text draws its baseline ~font_size below its position, so shift up by font_size
  // so that texts/labels of different font sizes at the same y line up
  for ( const auto &label : m_ui_data->m_labels )
  {
    TextColumn draw_line{ .self = *this,
                          .cache_key = "grimoire_" + label.name,
                          .origin = label.rect.position - sf::Vector2f{ 0.f, static_cast<float>( label.font_size ) },
                          .font_size = static_cast<unsigned int>( label.font_size ),
                          .line_height = static_cast<float>( label.font_size ) * 1.2f };
    if ( label.name == "page_label" )
    {
      draw_line( std::format( "{} of {}", m_current_page_idx + 1, grimoire_cmp->pages.size() ), sf::Color::Black );
      continue;
    }

    const auto [sub_page, action_key] = resolve_subpage( current_page, label.name );
    if ( not sub_page or not action_key.ends_with( "_action" ) ) { continue; }

    const auto action_it = sub_page->action_map.find( std::string( action_key ) );
    if ( action_it == sub_page->action_map.end() )
    {
      draw_line( "?????", sf::Color::Black );
      continue;
    }
    // one line per non-zero stat, or "none" if the action is discovered but has no effect
    const Cmp::BaseAction &action = action_it->second;
    const std::string tick_suffix = action.interval() > 0.f ? std::format( "pt/{:g}s", action.interval() ) : "";
    bool has_effect = false;
    const auto draw_stat = [&]( std::string_view stat_name, int value )
    {
      if ( value == 0 ) { return; }
      draw_line( std::format( "{}: {}{}", stat_name, value, tick_suffix ), sf::Color::Black );
      has_effect = true;
    };
    draw_stat( "health", action.health() );
    draw_stat( "fear", action.fear() );
    draw_stat( "despair", action.despair() );
    draw_stat( "infamy", action.infamy() );
    draw_stat( "luck", action.luck() );
    // only the toxidromes this action changes; a multi-type list (cures) shares one toxicity, so show it as "all"
    const Cmp::Toxicity::Toxidrome &toxidrome = action.toxidrome();
    std::vector<std::pair<std::string_view, int>> toxidrome_stats;
    const auto add_toxidrome = [&]( std::string_view name, std::optional<int> value )
    {
      if ( value.value_or( 0 ) != 0 ) { toxidrome_stats.emplace_back( name, *value ); }
    };
    add_toxidrome( "tachycardia", toxidrome.at<Cmp::Toxicity::Tachycardia>() );
    add_toxidrome( "bradycardia", toxidrome.at<Cmp::Toxicity::Bradycardia>() );
    add_toxidrome( "hallucinogen", toxidrome.at<Cmp::Toxicity::Hallucinogen>() );
    add_toxidrome( "hypoxia", toxidrome.at<Cmp::Toxicity::Hypoxia>() );
    add_toxidrome( "phototoxia", toxidrome.at<Cmp::Toxicity::Phototoxia>() );
    add_toxidrome( "venom", toxidrome.at<Cmp::Toxicity::Venom>() );
    add_toxidrome( "vertigo", toxidrome.at<Cmp::Toxicity::Vertigo>() );
    if ( toxidrome_stats.size() > 1 ) { draw_stat( "all", toxidrome_stats.front().second ); }
    else
    {
      for ( const auto &[name, value] : toxidrome_stats )
      {
        draw_stat( name, value );
      }
    }
    if ( not has_effect ) { draw_line( "none", sf::Color::Black ); }
  }
  for ( const auto &text : m_ui_data->m_texts )
  {
    TextColumn draw_line{ .self = *this,
                          .cache_key = "grimoire_" + text.name,
                          .origin = text.rect.position - sf::Vector2f{ 0.f, static_cast<float>( text.font_size ) },
                          .font_size = static_cast<unsigned int>( text.font_size ),
                          .line_height = static_cast<float>( text.font_size ) * 1.2f };
    const auto [sub_page, suffix] = resolve_subpage( current_page, text.name );
    if ( sub_page and suffix == "name" ) { draw_line( Sys::SpriteStore::instance().get( sub_page->sprite_key ).display_name(), sf::Color::Black ); }
    else { draw_line( text.value, sf::Color::Black ); }
  }
}

} // namespace Game::Sys
