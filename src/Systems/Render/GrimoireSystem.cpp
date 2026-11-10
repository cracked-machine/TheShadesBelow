#include <Audio/SoundBank.hpp>
#include <Components/Inventory/Grimoire.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Events/GrimoireDiscoveryEvent.hpp>
#include <Events/GrimoireNpcUpdateEvent.hpp>
#include <Events/ItemPickedUpEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <Systems/Render/GrimoireSystem.hpp>

#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Player.hpp>
#include <algorithm>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>
#include <format>
#include <optional>
#include <spdlog/spdlog.h>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace Game::Sys
{

namespace
{
constexpr float kToggleDebounceThreshold = 1.f;
constexpr float kNavigateDebounceThreshold = 0.5f;

//! @brief Map a grimoire_plants.json object name (e.g. "forage_carry_action") to the page's subpage for its
//! prefix and the remainder of the name (e.g. "carry_action").
//! @return The subpage, or nullptr if the name has no subpage prefix.
std::pair<const Cmp::Grimoire::SubPage *, std::string_view> resolve_subpage( const Cmp::Grimoire::PlantPage &page, std::string_view name )
{
  // the *_special_ prefixes must come before forage_/drop_ as the first matching prefix wins
  const std::pair<std::string_view, const Cmp::Grimoire::SubPage *> prefixes[] = { { "forage_special_", &page.forage_offering_subpage },
                                                                                   { "drop_special_", &page.drop_offering_subpage },
                                                                                   { "plant_", &page.plant_subpage },
                                                                                   { "forage_", &page.forage_subpage },
                                                                                   { "drop_", &page.drop_subpage } };
  for ( const auto &[prefix, sub_page] : prefixes )
  {
    if ( name.starts_with( prefix ) ) { return { sub_page, name.substr( prefix.size() ) }; }
  }
  return { nullptr, name };
}

//! @brief Map a grimoire_npc.json object name (e.g. "npc_spawn_action") to the NPC page's subpage and the remainder
//! of the name (e.g. "spawn_action").
//! @return The subpage, or nullptr if the name has no "npc_" prefix.
std::pair<const Cmp::Grimoire::SubPage *, std::string_view> resolve_subpage( const Cmp::Grimoire::NpcPage &page, std::string_view name )
{
  static constexpr std::string_view kPrefix = "npc_";
  if ( name.starts_with( kPrefix ) ) { return { &page.npc_subpage, name.substr( kPrefix.size() ) }; }
  return { nullptr, name };
}
} // namespace

GrimoireSystem::GrimoireSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : RenderSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&GrimoireSystem::on_player_action>( this );
  std::ignore = get_systems_event_queue().sink<Events::ItemPickedUpEvent>().connect<&GrimoireSystem::on_item_pickup_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::AddInventoryEvent>().connect<&GrimoireSystem::on_add_inventory_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::GrimoireItemUpdateEvent>().connect<&GrimoireSystem::on_grimoire_item_update_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::GrimoireNpcUpdateEvent>().connect<&GrimoireSystem::on_grimoire_npc_update_event>( this );

  m_plant_ui_data = std::make_unique<Render::UiData>( "res/ui/grimoire_plants.json" );
  m_npc_ui_data = std::make_unique<Render::UiData>( "res/ui/grimoire_npc.json" );
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
    // only plant pages hold items
    auto *plant_page = std::get_if<Cmp::Grimoire::PlantPage>( &grimoire_cmp->pages[page_idx] );
    if ( not plant_page ) { continue; }
    Cmp::Grimoire::PlantPage &page = *plant_page;
    for ( Cmp::Grimoire::SubPage *sub_page :
          { &page.plant_subpage, &page.forage_subpage, &page.drop_subpage, &page.forage_offering_subpage, &page.drop_offering_subpage } )
    {
      if ( sub_page->item_key == item_key ) { fn( page_idx, *sub_page ); }
    }
  }
}

void GrimoireSystem::on_item_pickup_event( const Events::ItemPickedUpEvent &ev ) { discover_item( ev.m_item ); }

void GrimoireSystem::on_add_inventory_event( const Events::AddInventoryEvent &ev ) { discover_item( ev.m_item ); }

void GrimoireSystem::discover_item( const Sys::ItemKey &item_key )
{
  const auto &item = Sys::ItemStore::instance().get( item_key );
  bool changed = false;
  for_each_subpage( item_key, [&]( std::size_t page_idx, Cmp::Grimoire::SubPage &sub_page )
  {
    if ( sub_page.sprite_key != item.sprite_type ) { changed = true; }
    sub_page.sprite_key = item.sprite_type;
    if ( item.has_action<Cmp::CarryAction>() )
    {
      sub_page.action_map.insert_or_assign( std::string( Cmp::CarryAction::kKey ), item.get_action<Cmp::CarryAction>() );
      if ( sub_page.discovered_actions.insert( std::string( Cmp::CarryAction::kKey ) ).second ) { changed = true; }
    }
    if ( not is_grimoire_overlay_open() ) { m_current_page_idx = page_idx; }
  } );
  // once per item, however many pages it appears on
  if ( changed ) { get_systems_event_queue().trigger( Events::GrimoireDiscoveryEvent( item_key ) ); }
}

void GrimoireSystem::on_grimoire_item_update_event( const Events::GrimoireItemUpdateEvent &ev )
{
  if ( ev.m_action_key == Cmp::ProximityAction::kKey and ev.m_action.is_zero() ) { return; }
  const auto &item = Sys::ItemStore::instance().get( ev.m_item );
  bool changed = false;
  for_each_subpage( ev.m_item, [&]( std::size_t page_idx, Cmp::Grimoire::SubPage &sub_page )
  {
    if ( sub_page.sprite_key != item.sprite_type ) { changed = true; }
    sub_page.sprite_key = item.sprite_type;
    sub_page.action_map.insert_or_assign( ev.m_action_key, ev.m_action );
    // repeating actions send an update on every application; only the first one is a discovery
    if ( sub_page.discovered_actions.insert( ev.m_action_key ).second ) { changed = true; }
    if ( not is_grimoire_overlay_open() ) { m_current_page_idx = page_idx; }
  } );
  // once per item, however many pages it appears on
  if ( changed ) { get_systems_event_queue().trigger( Events::GrimoireDiscoveryEvent( ev.m_item ) ); }
}

void GrimoireSystem::on_grimoire_npc_update_event( const Events::GrimoireNpcUpdateEvent &ev )
{
  if ( ev.m_action_key == Cmp::ProximityAction::kKey and ev.m_action.is_zero() ) { return; }
  auto *grimoire_cmp = find_grimoire();
  if ( not grimoire_cmp ) { return; }

  for ( std::size_t page_idx = 0; page_idx < grimoire_cmp->pages.size(); ++page_idx )
  {
    auto *npc_page = std::get_if<Cmp::Grimoire::NpcPage>( &grimoire_cmp->pages[page_idx] );
    if ( not npc_page or npc_page->npc_key != ev.m_npc ) { continue; }

    Cmp::Grimoire::SubPage &sub_page = npc_page->npc_subpage;
    const Sys::SpriteKey &sprite_key = Sys::NpcStore::instance().get( ev.m_npc ).sprite_type_list.front();
    bool changed = sub_page.sprite_key != sprite_key;
    sub_page.sprite_key = sprite_key;
    sub_page.action_map.insert_or_assign( ev.m_action_key, ev.m_action );
    // repeating actions send an update on every application; only the first one is a discovery
    if ( sub_page.discovered_actions.insert( ev.m_action_key ).second ) { changed = true; }
    if ( not is_grimoire_overlay_open() ) { m_current_page_idx = page_idx; }
    if ( changed ) { get_systems_event_queue().trigger( Events::GrimoireDiscoveryEvent( ev.m_npc ) ); }
    return;
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
    if ( grimoire_cmp->is_enabled )
    {
      // the mouse release is swallowed while paused, so abandon any draw now rather than leave it pending.
      // Cancel before pausing the effects so the draw sfx is stopped, not paused and later resumed.
      get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::CANCEL_BOW ) );
      m_sound_bank.pause_effects();
    }
    else { m_sound_bank.resume_effects(); }
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

std::string GrimoireSystem::npc_display_name( const std::string &npc_type )
{
  const auto npc_types = Sys::NpcStore::instance().get_all();
  if ( std::ranges::find( npc_types, Sys::NpcKey( npc_type ) ) == npc_types.end() ) { return npc_type; }
  const auto &npc = Sys::NpcStore::instance().get( Sys::NpcKey( npc_type ) );
  return Sys::SpriteStore::instance().get( npc.sprite_type_list.front() ).display_name();
}

std::string GrimoireSystem::reward_display_name( const Cmp::ActionReward &reward )
{
  if ( not reward.item.empty() )
  {
    const std::string quantity = reward.quantity > 1 ? std::format( "{}x ", reward.quantity ) : "";
    if ( reward.is_item_wildcard() )
    {
      // e.g. "item.jewelry*" -> "random jewelry"
      auto prefix = reward.item.str().substr( 0, reward.item.str().size() - 1 );
      if ( prefix.starts_with( "item." ) ) { prefix.erase( 0, std::string_view( "item." ).size() ); }
      return std::format( "{}random {}", quantity, prefix );
    }
    return quantity + Sys::SpriteStore::instance().get( Sys::ItemStore::instance().get( reward.item ).sprite_type ).display_name();
  }
  if ( not reward.loot.empty() ) { return Sys::SpriteStore::instance().get( reward.loot ).display_name(); }
  if ( reward.wealth > 0 ) { return std::format( "+{} wealth", reward.wealth ); }
  switch ( reward.altar )
  {
    case Cmp::ActionReward::AltarEffect::TALISMAN:
      return "lights altar";
    case Cmp::ActionReward::AltarEffect::BANISH:
      return "banishes ghosts";
    case Cmp::ActionReward::AltarEffect::EXTRA_LIFE:
      return "extra life";
    case Cmp::ActionReward::AltarEffect::CRYPT_KEY:
      return "unlocks crypt";
    case Cmp::ActionReward::AltarEffect::TOXIC_IMMUNITY:
      return std::format( "toxic immunity {}s", reward.tick );
    case Cmp::ActionReward::AltarEffect::NONE:
      break;
  }
  return "none";
}

void GrimoireSystem::render()
{
  // only draw the grimoire if its enabled
  const auto *grimoire_cmp = find_grimoire();
  if ( not grimoire_cmp or not grimoire_cmp->is_enabled or grimoire_cmp->pages.empty() ) return;

  const std::size_t page_count = grimoire_cmp->pages.size();
  std::visit(
      [&]( const auto &current_page )
      {
        using PageT = std::decay_t<decltype( current_page )>;
        constexpr bool is_npc_page = std::is_same_v<PageT, Cmp::Grimoire::NpcPage>;
        const Render::UiData &ui_data = is_npc_page ? *m_npc_ui_data : *m_plant_ui_data;
        // the layouts share object names (e.g. "page_label"), so keep their cached texts apart
        const std::string cache_prefix = is_npc_page ? "grimoire_npc_" : "grimoire_plant_";
        const SubPageResolver resolver = [&current_page]( std::string_view name ) { return resolve_subpage( current_page, name ); };
        render_outlines( ui_data );
        render_icons( ui_data, resolver );
        render_labels( ui_data, cache_prefix, resolver, page_count );
        render_texts( ui_data, cache_prefix, resolver );
      },
      grimoire_cmp->pages.at( m_current_page_idx ) );
}

void GrimoireSystem::render_outlines( const Render::UiData &ui_data )
{
  for ( const auto &outline : ui_data.m_outlines )
  {
    auto rect = sf::RectangleShape( outline.rect.size );
    rect.setPosition( outline.rect.position );
    rect.setFillColor( outline.fill_color );
    rect.setOutlineColor( outline.line_color );
    rect.setOutlineThickness( static_cast<float>( outline.line_thickness ) );
    rect.setRotation( outline.rotation );
    draw_screen( rect );
  }
}

void GrimoireSystem::render_icons( const Render::UiData &ui_data, const SubPageResolver &resolver )
{
  for ( const auto &icon : ui_data.m_icons )
  {
    const auto [sub_page, suffix] = resolver( icon.name );
    if ( not sub_page or suffix != "icon" ) { continue; }

    Sys::SpriteKey sprite_key = sub_page->sprite_key;
    RenderSystem::safe_render_sprite_screen( sprite_key, { icon.rect.position, Constants::kGridSizePxF }, 0,
                                             { static_cast<float>( icon.scale ), static_cast<float>( icon.scale ) } );
  }
}

void GrimoireSystem::render_labels( const Render::UiData &ui_data, const std::string &cache_prefix, const SubPageResolver &resolver,
                                    std::size_t page_count )
{
  // Tiled point y is the text baseline; sf::Text draws its baseline ~font_size below its position, so shift up by font_size
  // so that texts/labels of different font sizes at the same y line up
  for ( const auto &label : ui_data.m_labels )
  {
    TextColumn draw_line{ .self = *this,
                          .cache_key = cache_prefix + label.name,
                          .origin = label.rect.position - sf::Vector2f{ 0.f, static_cast<float>( label.font_size ) },
                          .font_size = static_cast<unsigned int>( label.font_size ),
                          .line_height = static_cast<float>( label.font_size ) * 1.2f };
    if ( label.name == "page_label" )
    {
      draw_line( std::format( "{} of {}", m_current_page_idx + 1, page_count ), sf::Color::Black );
      continue;
    }

    const auto [sub_page, action_key] = resolver( label.name );
    if ( not sub_page or not action_key.ends_with( "_action" ) ) { continue; }

    const auto action_it = sub_page->action_map.find( std::string( action_key ) );
    if ( action_it == sub_page->action_map.end() )
    {
      draw_line( "?????", sf::Color::Black );
      continue;
    }
    // reward, stats then side effects, or "none" if the action is discovered but has no effect
    const Cmp::BaseAction &action = action_it->second;
    const bool has_reward = render_action_reward( draw_line, action );
    const bool has_stats = render_action_stats( draw_line, action );
    const bool has_side_effects = render_action_side_effects( draw_line, action );
    if ( not has_reward and not has_stats and not has_side_effects ) { draw_line( "none", sf::Color::Black ); }
  }
}

bool GrimoireSystem::render_action_reward( TextColumn &draw_line, const Cmp::BaseAction &action )
{
  // the reward goes first so it sits alongside the action's header text, e.g. "Offering: (Elder Flute)"
  const auto reward = action.reward();
  if ( not reward ) { return false; }
  draw_line( std::format( "({})", reward_display_name( *reward ) ), sf::Color::Black );
  return true;
}

bool GrimoireSystem::render_action_stats( TextColumn &draw_line, const Cmp::BaseAction &action )
{
  // one line per non-zero stat
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
  draw_stat( "anima", action.anima() );
  // only the toxidromes this action changes; when it changes every toxidrome (a "type": "all" cure, which shares one
  // toxicity) show a single "all" line instead
  const Cmp::Toxicity::Toxidrome &toxidrome = action.toxidrome();
  const std::pair<std::string_view, std::optional<int>> all_toxidromes[] = { { "tachycardia", toxidrome.at<Cmp::Toxicity::Tachycardia>() },
                                                                             { "bradycardia", toxidrome.at<Cmp::Toxicity::Bradycardia>() },
                                                                             { "hallucinogen", toxidrome.at<Cmp::Toxicity::Hallucinogen>() },
                                                                             { "hypoxia", toxidrome.at<Cmp::Toxicity::Hypoxia>() },
                                                                             { "phototoxia", toxidrome.at<Cmp::Toxicity::Phototoxia>() },
                                                                             { "venom", toxidrome.at<Cmp::Toxicity::Venom>() },
                                                                             { "vertigo", toxidrome.at<Cmp::Toxicity::Vertigo>() } };
  std::vector<std::pair<std::string_view, int>> toxidrome_stats;
  for ( const auto &[name, value] : all_toxidromes )
  {
    if ( value.value_or( 0 ) != 0 ) { toxidrome_stats.emplace_back( name, *value ); }
  }
  if ( toxidrome_stats.size() == std::size( all_toxidromes ) ) { draw_stat( "all", toxidrome_stats.front().second ); }
  else
  {
    for ( const auto &[name, value] : toxidrome_stats )
    {
      draw_stat( name, value );
    }
  }
  return has_effect;
}

bool GrimoireSystem::render_action_side_effects( TextColumn &draw_line, const Cmp::BaseAction &action )
{
  // side effects, with their chance of firing
  const Cmp::SideEffects &effects = action.side_effects();
  const int player_luck = Utils::Player::get_stats( reg() ).luck();
  const auto draw_effect = [&]( std::string_view label, float chance )
  { draw_line( std::format( "{}: {:.0f}%", label, chance * 100.f ), sf::Color::Black ); };
  if ( effects.spawn_npc )
  {
    std::string label = std::format( "summons {}", npc_display_name( effects.spawn_npc->npc_type ) );
    if ( const auto &condition = effects.spawn_npc->condition ) { label += std::format( " if {} {}", condition->stat_name, condition->value ); }
    draw_effect( label, effects.spawn_npc->chance( player_luck ) );
  }
  if ( effects.lightning ) { draw_effect( "lightning", effects.lightning->chance( player_luck ) ); }
  if ( effects.movement )
  {
    using PlayerType = Cmp::MovementEffect::Player::Type;
    using NpcType = Cmp::MovementEffect::Npc::Type;
    if ( const auto &player = effects.movement->player )
    {
      const std::string_view type = player->type == PlayerType::SLOW ? "slow" : "drunk";
      draw_effect( std::format( "player {} {}s", type, player->tick ), effects.movement->chance( player_luck ) );
    }
    if ( const auto &npc = effects.movement->npc )
    {
      const std::string_view type = npc->type == NpcType::SLOW ? "slow" : npc->type == NpcType::DRUNK ? "drunk" : "repel";
      draw_effect( std::format( "npc {} {}s", type, npc->tick ), effects.movement->chance( player_luck ) );
    }
  }
  if ( effects.toxic_immunity )
  {
    draw_effect( std::format( "toxic immunity {}s", effects.toxic_immunity->tick ), effects.toxic_immunity->chance( player_luck ) );
  }
  if ( effects.kill_npc )
  {
    std::string names;
    for ( const auto &npc_type : effects.kill_npc->npc_types )
    {
      names += std::format( "{}{}", names.empty() ? "" : ", ", npc_display_name( npc_type.str() ) );
    }
    draw_effect( std::format( "kills {}", names.empty() ? "npcs" : names ), effects.kill_npc->chance( player_luck ) );
  }
  return not effects.empty();
}

void GrimoireSystem::render_texts( const Render::UiData &ui_data, const std::string &cache_prefix, const SubPageResolver &resolver )
{
  for ( const auto &text : ui_data.m_texts )
  {
    TextColumn draw_line{ .self = *this,
                          .cache_key = cache_prefix + text.name,
                          .origin = text.rect.position - sf::Vector2f{ 0.f, static_cast<float>( text.font_size ) },
                          .font_size = static_cast<unsigned int>( text.font_size ),
                          .line_height = static_cast<float>( text.font_size ) * 1.2f };
    const auto [sub_page, suffix] = resolver( text.name );
    if ( sub_page and suffix == "name" ) { draw_line( Sys::SpriteStore::instance().get( sub_page->sprite_key ).display_name(), sf::Color::Black ); }
    else { draw_line( text.value, sf::Color::Black ); }
  }
}
} // namespace Game::Sys
