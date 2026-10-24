#include <Components/Inventory/Grimoire.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <Systems/Render/GrimoireSystem.hpp>

#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <algorithm>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>
#include <format>
#include <spdlog/spdlog.h>
#include <stdexcept>

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

  m_ui_data = std::make_unique<Render::UiData>( "res/ui/grimoire.json" );
  m_all_items = Sys::ItemStore::instance().get_pattern( "^item\\.plant\\." );
  if ( m_all_items.empty() ) { throw std::runtime_error( "GrimoireSystem: No plant items found in ItemStore" ); }
  std::ranges::sort( m_all_items, {}, &ItemKey::str );
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

  if ( not grimoire_cmp->is_enabled or m_navigate_debounce.asSeconds() < kNavigateDebounceThreshold ) { return; }

  if ( ev.action == Events::PlayerActionEvent::GameActions::GRIMOIRE_PREV )
  {
    m_current_idx = ( m_current_idx + m_all_items.size() - 1 ) % m_all_items.size();
    m_navigate_debounce = sf::Time::Zero;
  }
  else if ( ev.action == Events::PlayerActionEvent::GameActions::GRIMOIRE_NEXT )
  {
    m_current_idx = ( m_current_idx + 1 ) % m_all_items.size();
    m_navigate_debounce = sf::Time::Zero;
  }
}

void GrimoireSystem::render()
{
  // only draw the grimoire if its enabled
  const auto *grimoire_cmp = find_grimoire();
  if ( not grimoire_cmp or not grimoire_cmp->is_enabled ) return;

  const auto &item = Sys::ItemStore::instance().get( m_all_items.at( m_current_idx ) );

  for ( const auto &outline : m_ui_data->m_outlines )
  {
    if ( outline.name == "main_outline" )
    {
      auto rect = sf::RectangleShape( outline.rect.size );
      rect.setPosition( outline.rect.position );
      rect.setFillColor( outline.fill_color );
      rect.setOutlineColor( outline.line_color );
      rect.setOutlineThickness( static_cast<float>( outline.line_thickness ) );
      draw_screen( rect );
    }
  }
  for ( const auto &icon : m_ui_data->m_icons )
  {
    if ( icon.name == "plant_icon" )
    {
      RenderSystem::safe_render_sprite_screen( item.sprite_type, { icon.rect.position, Constants::kGridSizePxF }, 0,
                                               { static_cast<float>( icon.scale ), static_cast<float>( icon.scale ) } );
    }
  }
  for ( const auto &label : m_ui_data->m_labels )
  {
    const auto font_size = static_cast<unsigned int>( label.font_size );
    const auto line_height = static_cast<float>( label.font_size ) * 1.2f;
    if ( label.name == "page_label" )
    {
      TextColumn draw_line{ .self = *this, .cache_key = "grimoire_page_num", .origin = label.rect.position, .font_size = font_size, .line_height = line_height };
      draw_line( std::format( "{} of {}", m_current_idx + 1, m_all_items.size() ), sf::Color::Black );
    }
    else if ( label.name == "plant_label" )
    {
      TextColumn draw_line{ .self = *this, .cache_key = "grimoire_plant_name", .origin = label.rect.position, .font_size = font_size, .line_height = line_height };
      const auto &sprite = Sys::SpriteStore::instance().get( item.sprite_type );
      draw_line( sprite.display_name(), sf::Color::Black );
    }
  }
}

} // namespace Game::Sys
