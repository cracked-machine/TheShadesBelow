#include <Audio/SoundBank.hpp>
#include <Components/Inventory/Grimoire.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Events/ItemPickedUpEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <Systems/Render/GrimoireSystem.hpp>

#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>
#include <format>
#include <iterator>
#include <spdlog/spdlog.h>

namespace Game::Sys
{

namespace
{
constexpr float kToggleDebounceThreshold = 1.f;
constexpr float kNavigateDebounceThreshold = 0.5f;
} // namespace

GrimoireSystem::GrimoireSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : RenderSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&GrimoireSystem::on_player_action>( this );
  std::ignore = get_systems_event_queue().sink<Events::ItemPickedUpEvent>().connect<&GrimoireSystem::on_item_pickup_event>( this );

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

Cmp::Grimoire::PageMap::const_iterator GrimoireSystem::current_page( const Cmp::Grimoire &grimoire ) const
{
  auto it = grimoire.pages.find( m_item_key );
  return it != grimoire.pages.end() ? it : grimoire.pages.begin();
}

void GrimoireSystem::on_item_pickup_event( const Events::ItemPickedUpEvent &ev )
{
  if ( ev.m_item.contains( "item.plant" ) )
  {
    auto *grimoire_cmp = find_grimoire();
    if ( not grimoire_cmp ) { return; }

    const auto &item = Sys::ItemStore::instance().get( ev.m_item );
    grimoire_cmp->pages[item.item_type] = Cmp::Grimoire::Page{
        .sprite_key = item.sprite_type,
        .action_map = Cmp::Grimoire::ActionMap{ { std::string( "carry_action" ), item.get_action<Cmp::CarryAction>() } } };

    SPDLOG_INFO( "ItemPickedUpEvent: {} ", ev.m_item );
  }
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

  const auto &pages = grimoire_cmp->pages;
  if ( ev.action == Events::PlayerActionEvent::GameActions::GRIMOIRE_PREV )
  {
    auto it = current_page( *grimoire_cmp );
    it = ( it == pages.begin() ) ? std::prev( pages.end() ) : std::prev( it );
    m_item_key = it->first;
    m_navigate_debounce = sf::Time::Zero;
    m_sound_bank.get_effect( "page_turn" ).play();
  }
  else if ( ev.action == Events::PlayerActionEvent::GameActions::GRIMOIRE_NEXT )
  {
    auto it = std::next( current_page( *grimoire_cmp ) );
    if ( it == pages.end() ) { it = pages.begin(); }
    m_item_key = it->first;
    m_navigate_debounce = sf::Time::Zero;
    m_sound_bank.get_effect( "page_turn" ).play();
  }
}

void GrimoireSystem::render()
{
  // only draw the grimoire if its enabled
  const auto *grimoire_cmp = find_grimoire();
  if ( not grimoire_cmp or not grimoire_cmp->is_enabled or grimoire_cmp->pages.empty() ) return;

  const auto page_it = current_page( *grimoire_cmp );
  const auto &page = page_it->second;
  const auto page_num = std::distance( grimoire_cmp->pages.begin(), page_it ) + 1;

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
    if ( icon.name == "plant_icon" )
    {
      if ( page.sprite_key.empty() )
      {
        RenderSystem::safe_render_sprite_screen( "sprite.plant.unknown", { icon.rect.position, Constants::kGridSizePxF }, 0,
                                                 { static_cast<float>( icon.scale ), static_cast<float>( icon.scale ) } );
      }
      else
      {
        RenderSystem::safe_render_sprite_screen( page.sprite_key, { icon.rect.position, Constants::kGridSizePxF }, 0,
                                                 { static_cast<float>( icon.scale ), static_cast<float>( icon.scale ) } );
      }
    }
  }
  for ( const auto &label : m_ui_data->m_labels )
  {
    if ( label.name == "page_label" )
    {
      const auto font_size = static_cast<unsigned int>( label.font_size );
      const auto line_height = static_cast<float>( label.font_size ) * 1.2f;
      TextColumn draw_line{
          .self = *this, .cache_key = "grimoire_page_num", .origin = label.rect.position, .font_size = font_size, .line_height = line_height };
      draw_line( std::format( "{} of {}", page_num, grimoire_cmp->pages.size() ), sf::Color::Black );
    }
    else if ( label.name.contains( "action" ) )
    {
      TextColumn draw_line{ .self = *this,
                            .cache_key = "grimoire_action",
                            .origin = label.rect.position,
                            .font_size = static_cast<unsigned int>( label.font_size ),
                            .line_height = label.font_size * 1.2f };
      if ( page.action_map.contains( label.name ) )
      {
        const Cmp::BaseAction &action = page.action_map.at( label.name );
        if ( action.health() != 0 ) { draw_line( std::format( "health: {}", action.health() ), sf::Color::Black ); }
        else if ( action.fear() != 0 ) { draw_line( std::format( "fear: {}", action.fear() ), sf::Color::Black ); }
        else if ( action.despair() != 0 ) { draw_line( std::format( "despair: {}", action.despair() ), sf::Color::Black ); }
        else if ( action.infamy() != 0 ) { draw_line( std::format( "infamy: {}", action.infamy() ), sf::Color::Black ); }
        else if ( action.luck() != 0 ) { draw_line( std::format( "luck: {}", action.luck() ), sf::Color::Black ); }
        else { draw_line( "?????", sf::Color::Black ); }
      }
      else { draw_line( "?????", sf::Color::Black ); }
    }
  }
  for ( const auto &text : m_ui_data->m_texts )
  {
    TextColumn draw_line{ .self = *this,
                          .cache_key = "grimoire_text",
                          .origin = text.rect.position,
                          .font_size = static_cast<unsigned int>( text.font_size ),
                          .line_height = text.font_size * 1.2f };
    if ( text.name == "plant_name" and not page.sprite_key.empty() )
    {
      draw_line( Sys::SpriteStore::instance().get( page.sprite_key ).display_name(), sf::Color::Black );
    }
    else { draw_line( text.value, sf::Color::Black ); }
  }
}

} // namespace Game::Sys
