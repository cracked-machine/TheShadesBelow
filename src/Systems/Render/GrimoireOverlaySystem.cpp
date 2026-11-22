#include <Audio/SoundBank.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Inventory/Grimoire.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/NoRender.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Persistent/GrimoireHoverFollowsMouse.hpp>
#include <Components/Position.hpp>
#include <Components/Stats/ActionReward.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/SideEffects.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Events/GrimoireDiscoveryEvent.hpp>
#include <Events/GrimoireNpcUpdateEvent.hpp>
#include <Events/ItemPickedUpEvent.hpp>
#include <Events/OverlayEvent.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Window/Mouse.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/GrimoireOverlaySystem.hpp>
#include <Systems/Render/OverlayArbitrationSystem.hpp>

#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/ImGuiTheme.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>
#include <algorithm>
#include <array>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>
#include <format>
#include <imgui.h>
#include <optional>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string_view>
#include <utility>
#include <vector>

namespace Game::Sys
{

GrimoireOverlaySystem::GrimoireOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseOverlaySystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::OverlayEvent>().connect<&GrimoireOverlaySystem::on_overlay_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::ItemPickedUpEvent>().connect<&GrimoireOverlaySystem::on_item_pickup_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::AddInventoryEvent>().connect<&GrimoireOverlaySystem::on_add_inventory_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::GrimoireItemUpdateEvent>().connect<&GrimoireOverlaySystem::on_grimoire_item_update_event>(
      this );
  std::ignore = get_systems_event_queue().sink<Events::GrimoireNpcUpdateEvent>().connect<&GrimoireOverlaySystem::on_grimoire_npc_update_event>(
      this );

  m_plant_ui_layout = std::make_unique<Render::UiLayout>( "res/ui/grimoire_plants.json" );
  m_npc_ui_layout = std::make_unique<Render::UiLayout>( "res/ui/grimoire_npc.json" );
  m_plant_hover_ui_layout = std::make_unique<Render::UiLayout>( "res/ui/grimoire_plant_hover.json" );
  m_npc_hover_ui_layout = std::make_unique<Render::UiLayout>( "res/ui/grimoire_npc_hover.json" );
}

bool GrimoireOverlaySystem::is_open() const
{
  auto grimoire_view = reg().view<const Cmp::Grimoire>();
  if ( grimoire_view.empty() ) { return false; }
  return grimoire_view.get<const Cmp::Grimoire>( grimoire_view.front() ).is_enabled;
}

void GrimoireOverlaySystem::open()
{
  auto *grimoire_cmp = find_grimoire();
  if ( not grimoire_cmp or grimoire_cmp->is_enabled ) { return; }
  grimoire_cmp->is_enabled = true;
  // ImGui remembers the tab last clicked, so select the one a discovery may have switched to
  m_select_tab = true;
  on_modal_open();
}

void GrimoireOverlaySystem::close()
{
  auto *grimoire_cmp = find_grimoire();
  if ( not grimoire_cmp or not grimoire_cmp->is_enabled ) { return; }
  grimoire_cmp->is_enabled = false;
  on_modal_close();
}

Cmp::Grimoire *GrimoireOverlaySystem::find_grimoire()
{
  auto grimoire_view = reg().view<Cmp::Grimoire>();
  if ( grimoire_view.empty() ) { return nullptr; }
  return &grimoire_view.get<Cmp::Grimoire>( grimoire_view.front() );
}

std::size_t GrimoireOverlaySystem::page_count( const Cmp::Grimoire &grimoire_cmp, Cmp::Grimoire::Tab tab )
{
  return tab == Cmp::Grimoire::Tab::NPCS ? grimoire_cmp.npc_pages.size() : grimoire_cmp.plant_pages.size();
}

void GrimoireOverlaySystem::for_each_subpage( const Sys::ItemKey &item_key, const std::function<void( std::size_t, Cmp::Grimoire::SubPage & )> &fn )
{
  auto *grimoire_cmp = find_grimoire();
  if ( not grimoire_cmp or item_key.empty() ) { return; }

  for ( std::size_t page_idx = 0; page_idx < grimoire_cmp->plant_pages.size(); ++page_idx )
  {
    Cmp::Grimoire::PlantPage &page = grimoire_cmp->plant_pages[page_idx];
    for ( Cmp::Grimoire::SubPage *sub_page : { &page.plant_subpage, &page.forage_subpage, &page.drop_subpage, &page.plant_offering_subpage,
                                               &page.forage_offering_subpage, &page.drop_offering_subpage } )
    {
      if ( sub_page->item_key == item_key ) { fn( page_idx, *sub_page ); }
    }
  }
}

void GrimoireOverlaySystem::show_page_when_closed( Cmp::Grimoire::Tab tab, std::size_t page_idx )
{
  if ( is_open() ) { return; }
  m_tab = tab;
  m_page_idx[tab_index( tab )] = page_idx;
}

void GrimoireOverlaySystem::on_item_pickup_event( const Events::ItemPickedUpEvent &ev ) { discover_item( ev.m_item ); }

void GrimoireOverlaySystem::on_add_inventory_event( const Events::AddInventoryEvent &ev ) { discover_item( ev.m_item ); }

void GrimoireOverlaySystem::discover_item( const Sys::ItemKey &item_key )
{
  const auto &item = Sys::ItemStore::instance().get( item_key );
  bool changed = false;
  for_each_subpage( item_key, [&]( std::size_t page_idx, Cmp::Grimoire::SubPage &sub_page )
  {
    if ( sub_page.sprite_key != item.sprite_type or sub_page.sprite_index != item.icon_sprite_index() ) { changed = true; }
    sub_page.sprite_key = item.sprite_type;
    sub_page.sprite_index = item.icon_sprite_index();
    if ( item.has_action<Cmp::CarryAction>() )
    {
      sub_page.action_map.insert_or_assign( std::string( Cmp::CarryAction::kKey ), item.get_action<Cmp::CarryAction>() );
      if ( sub_page.discovered_actions.insert( std::string( Cmp::CarryAction::kKey ) ).second ) { changed = true; }
    }
    show_page_when_closed( Cmp::Grimoire::Tab::PLANTS, page_idx );
  } );
  // once per item, however many pages it appears on
  if ( changed ) { get_systems_event_queue().trigger( Events::GrimoireDiscoveryEvent( item_key ) ); }
}

void GrimoireOverlaySystem::on_grimoire_item_update_event( const Events::GrimoireItemUpdateEvent &ev )
{
  if ( ev.m_action_key == Cmp::ProximityAction::kKey and ev.m_action.is_zero() ) { return; }
  const auto &item = Sys::ItemStore::instance().get( ev.m_item );
  bool changed = false;
  for_each_subpage( ev.m_item, [&]( std::size_t page_idx, Cmp::Grimoire::SubPage &sub_page )
  {
    if ( sub_page.sprite_key != item.sprite_type or sub_page.sprite_index != item.icon_sprite_index() ) { changed = true; }
    sub_page.sprite_key = item.sprite_type;
    sub_page.sprite_index = item.icon_sprite_index();
    sub_page.action_map.insert_or_assign( ev.m_action_key, ev.m_action );
    // repeating actions send an update on every application; only the first one is a discovery
    if ( sub_page.discovered_actions.insert( ev.m_action_key ).second ) { changed = true; }
    show_page_when_closed( Cmp::Grimoire::Tab::PLANTS, page_idx );
  } );
  // once per item, however many pages it appears on
  if ( changed ) { get_systems_event_queue().trigger( Events::GrimoireDiscoveryEvent( ev.m_item ) ); }
}

void GrimoireOverlaySystem::on_grimoire_npc_update_event( const Events::GrimoireNpcUpdateEvent &ev )
{
  if ( ev.m_action_key == Cmp::ProximityAction::kKey and ev.m_action.is_zero() ) { return; }
  auto *grimoire_cmp = find_grimoire();
  if ( not grimoire_cmp ) { return; }

  const auto page_it = std::ranges::find( grimoire_cmp->npc_pages, ev.m_npc, &Cmp::Grimoire::NpcPage::npc_key );
  if ( page_it == grimoire_cmp->npc_pages.end() ) { return; }

  Cmp::Grimoire::SubPage &sub_page = page_it->npc_subpage;
  const Sys::SpriteKey &sprite_key = Sys::NpcStore::instance().get( ev.m_npc ).sprite_type_list.front();
  bool changed = sub_page.sprite_key != sprite_key;
  sub_page.sprite_key = sprite_key;
  sub_page.sprite_index = 0;
  sub_page.action_map.insert_or_assign( ev.m_action_key, ev.m_action );
  // repeating actions send an update on every application; only the first one is a discovery
  if ( sub_page.discovered_actions.insert( ev.m_action_key ).second ) { changed = true; }
  show_page_when_closed( Cmp::Grimoire::Tab::NPCS, static_cast<std::size_t>( page_it - grimoire_cmp->npc_pages.begin() ) );
  if ( changed ) { get_systems_event_queue().trigger( Events::GrimoireDiscoveryEvent( ev.m_npc ) ); }
}

void GrimoireOverlaySystem::on_overlay_event( const Events::OverlayEvent &ev )
{
  using Action = Events::OverlayEvent::OverlayActions;
  if ( ev.action != Action::GRIMOIRE_PREV and ev.action != Action::GRIMOIRE_NEXT ) { return; }

  const auto *grimoire_cmp = find_grimoire();
  if ( not grimoire_cmp or not grimoire_cmp->is_enabled ) { return; }
  const std::size_t count = page_count( *grimoire_cmp, m_tab );
  if ( count == 0 ) { return; }

  constexpr float kNavigateDebounceThreshold = 0.5f;
  if ( m_navigate_debounce.asSeconds() < kNavigateDebounceThreshold ) { return; }

  // pages turn, and wrap around, within the current tab only
  std::size_t &page_idx = m_page_idx[tab_index( m_tab )];
  if ( ev.action == Action::GRIMOIRE_PREV ) { page_idx = ( page_idx - 1 + count ) % count; }
  else { page_idx = ( page_idx + 1 ) % count; }
  m_navigate_debounce = sf::Time::Zero;
  m_sound_bank.get_effect( "page_turn" ).play();
}

std::string GrimoireOverlaySystem::npc_display_name( const std::string &npc_type )
{
  const auto npc_types = Sys::NpcStore::instance().get_all();
  if ( std::ranges::find( npc_types, Sys::NpcKey( npc_type ) ) == npc_types.end() ) { return npc_type; }
  const auto &npc = Sys::NpcStore::instance().get( Sys::NpcKey( npc_type ) );
  return Sys::SpriteStore::instance().get( npc.sprite_type_list.front() ).display_name();
}

std::string GrimoireOverlaySystem::reward_display_name( const Cmp::ActionReward &reward )
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

void GrimoireOverlaySystem::update( sf::Time dt )
{
  m_navigate_debounce += dt;

  const auto *grimoire_cmp = find_grimoire();
  if ( not grimoire_cmp ) { return; }
  const bool grimoire_open = grimoire_cmp->is_enabled;

  // NPCs move and are drawn over plants, so an NPC in front of a plant wins
  const Cmp::Grimoire::NpcPage *hover_npc_page = nullptr;
  const Cmp::Grimoire::PlantPage *hover_plant_page = nullptr;
  if ( not grimoire_open and OverlayArbitrationSystem::hover_allowed( reg() ) )
  {
    hover_npc_page = find_hovered_npc_page( *grimoire_cmp );
    if ( not hover_npc_page ) { hover_plant_page = find_hovered_plant_page( *grimoire_cmp ); }
  }
  if ( not grimoire_open and not hover_npc_page and not hover_plant_page ) { return; }

  ImGui::PushFont( Utils::imgui_grimoire_font() );
  if ( grimoire_open ) { render_grimoire( *grimoire_cmp ); }
  else if ( hover_npc_page ) { render_hover( *m_npc_hover_ui_layout, "##grimoire_npc_hover", *hover_npc_page ); }
  else { render_hover( *m_plant_hover_ui_layout, "##grimoire_plant_hover", *hover_plant_page ); }
  ImGui::PopFont();
}

bool GrimoireOverlaySystem::begin_layout_window( const Render::UiLayout &ui_layout, const char *id, int extra_flags )
{
  const auto main_outline_it = std::ranges::find( ui_layout.m_outlines, std::string( "main_outline" ), &Render::UiLayout::Outline::name );
  if ( main_outline_it != ui_layout.m_outlines.end() )
  {
    ImGui::SetNextWindowPos( ImVec2( main_outline_it->rect.position.x, main_outline_it->rect.position.y ) );
    ImGui::SetNextWindowSize( ImVec2( main_outline_it->rect.size.x, main_outline_it->rect.size.y ) );
  }
  else
  {
    ImGui::SetNextWindowPos( ImVec2( 0.f, 0.f ) );
    ImGui::SetNextWindowSize( ImGui::GetIO().DisplaySize );
  }
  constexpr ImGuiWindowFlags kWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                                            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove;
  return ImGui::Begin( id, nullptr, kWindowFlags | extra_flags );
}

void GrimoireOverlaySystem::render_grimoire( const Cmp::Grimoire &grimoire_cmp )
{
  // the layouts are authored at their Tiled map size; rescale them if the window resolution has changed (no-op otherwise)
  m_npc_ui_layout->sync_to( m_window.getSize() );
  m_plant_ui_layout->sync_to( m_window.getSize() );

  // one invisible window over the layout's main_outline holds the tab bar and the tables, each placed by its layout
  // object. Widgets are clipped to it, and the tab bar stretches to its right edge. Without a main_outline it covers the screen.
  const Render::UiLayout &current_ui_layout = m_tab == Cmp::Grimoire::Tab::NPCS ? *m_npc_ui_layout : *m_plant_ui_layout;
  if ( begin_layout_window( current_ui_layout, "##grimoire", ImGuiWindowFlags_None ) )
  {
    // the tab bar first, so a click this frame shows the newly selected tab's page straight away. ImGui only draws when
    // OverlayArbitrationSystem renders the shared frame, so it still ends up on top of the SFML parts of the page.
    render_tab_bar( current_ui_layout );

    const std::size_t count = page_count( grimoire_cmp, m_tab );
    std::size_t &page_idx = m_page_idx[tab_index( m_tab )];
    if ( count > 0 )
    {
      page_idx = std::min( page_idx, count - 1 );
      if ( m_tab == Cmp::Grimoire::Tab::NPCS ) { render_page( *m_npc_ui_layout, grimoire_cmp.npc_pages.at( page_idx ), count ); }
      else { render_page( *m_plant_ui_layout, grimoire_cmp.plant_pages.at( page_idx ), count ); }
    }
  }
  ImGui::End();
}

template <typename PageT>
void GrimoireOverlaySystem::render_hover( Render::UiLayout &ui_layout, const char *window_id, const PageT &page )
{
  // the page is read from the live grimoire every frame, so a discovery made while hovering shows straight away
  const sf::Vector2u window_size = m_window.getSize();
  const bool follow_mouse = PersistSystem::get<Cmp::Persist::GrimoireHoverFollowsMouse>( reg() ).get_value();
  ui_layout.sync_to( window_size, follow_mouse ? hover_anchor( ui_layout, window_size ) : sf::Vector2f{ 0.f, 0.f } );

  // display only: the overlay must never take the mouse from the game
  if ( begin_layout_window( ui_layout, window_id, ImGuiWindowFlags_NoInputs ) )
  {
    // the hover layouts have no page_label, so the page count is unused
    render_page( ui_layout, page, 0 );
  }
  ImGui::End();
}

std::optional<sf::FloatRect> GrimoireOverlaySystem::mouse_bounds_in_window() const
{
  const sf::Vector2i mouse_pixel_pos = sf::Mouse::getPosition( m_window );
  const sf::Vector2u window_size = m_window.getSize();
  if ( mouse_pixel_pos.x < 0 or mouse_pixel_pos.y < 0 or mouse_pixel_pos.x >= static_cast<int>( window_size.x ) or
       mouse_pixel_pos.y >= static_cast<int>( window_size.y ) )
  {
    return std::nullopt;
  }
  return Utils::get_mouse_bounds_in_gameview( m_window, RenderSystem::get_world_view() );
}

const Cmp::Grimoire::PlantPage *GrimoireOverlaySystem::find_hovered_plant_page( const Cmp::Grimoire &grimoire_cmp ) const
{
  const auto mouse_bounds = mouse_bounds_in_window();
  if ( not mouse_bounds ) { return nullptr; }
  for ( auto [plant_entt, plant_mb_cmp, plant_item_cmp] : reg().view<Cmp::PlantMultiBlock, Cmp::WorldItem>().each() )
  {
    if ( not mouse_bounds->findIntersection( plant_mb_cmp ) ) { continue; }
    const auto page_it = std::ranges::find( grimoire_cmp.plant_pages, plant_item_cmp.item_type,
                                            []( const Cmp::Grimoire::PlantPage &page ) -> const Sys::ItemKey &
    { return page.plant_subpage.item_key; } );
    if ( page_it != grimoire_cmp.plant_pages.end() ) { return &*page_it; }
  }
  return nullptr;
}

const Cmp::Grimoire::NpcPage *GrimoireOverlaySystem::find_hovered_npc_page( const Cmp::Grimoire &grimoire_cmp ) const
{
  const auto mouse_bounds = mouse_bounds_in_window();
  if ( not mouse_bounds ) { return nullptr; }
  // hidden NPCs can't be hovered. NPC containers (e.g. bones piles) have no Cmp::Npc::NPC, so they aren't given away.
  for ( auto [npc_entt, npc_cmp, npc_pos_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position>( entt::exclude<Cmp::NoRender> ).each() )
  {
    if ( not mouse_bounds->findIntersection( npc_pos_cmp ) ) { continue; }
    const auto page_it = std::ranges::find( grimoire_cmp.npc_pages, npc_cmp.npc_type, &Cmp::Grimoire::NpcPage::npc_key );
    if ( page_it != grimoire_cmp.npc_pages.end() ) { return &*page_it; }
  }
  return nullptr;
}

sf::Vector2f GrimoireOverlaySystem::hover_anchor( Render::UiLayout &ui_layout, sf::Vector2u window_size )
{
  // rescale for the window at the current anchor first (a no-op unless the resolution changed), so the panel's size and
  // its position without an anchor can be read back from the layout
  const sf::Vector2f current_anchor = ui_layout.anchor_offset();
  ui_layout.sync_to( window_size, current_anchor );
  const auto &outlines = ui_layout.m_outlines;
  const auto main_outline_it = std::ranges::find( outlines, std::string( "main_outline" ), &Render::UiLayout::Outline::name );
  if ( main_outline_it == outlines.end() ) { return { 0.f, 0.f }; }

  const sf::Vector2f panel_size = main_outline_it->rect.size;
  const sf::Vector2f unanchored_pos = main_outline_it->rect.position - current_anchor;
  // keep the border, drawn outside the rect, on screen as well
  const float border = static_cast<float>( std::max( 0, main_outline_it->line_thickness ) );
  //! @brief Gap between the mouse cursor and the hover overlay when it follows the mouse, in design-space pixels.
  constexpr float kHoverCursorGapPx = 24.f;
  const float gap = kHoverCursorGapPx * ui_layout.scale();
  const sf::Vector2f mouse( sf::Mouse::getPosition( m_window ) );
  const sf::Vector2f window( window_size );

  // below-right of the cursor, or flipped to the other side where it would leave the window
  sf::Vector2f pos = mouse + sf::Vector2f{ gap, gap };
  if ( pos.x + panel_size.x + border > window.x ) { pos.x = mouse.x - gap - panel_size.x; }
  if ( pos.y + panel_size.y + border > window.y ) { pos.y = mouse.y - gap - panel_size.y; }
  pos.x = std::clamp( pos.x, border, std::max( border, window.x - panel_size.x - border ) );
  pos.y = std::clamp( pos.y, border, std::max( border, window.y - panel_size.y - border ) );
  return pos - unanchored_pos;
}

void GrimoireOverlaySystem::render_tab_bar( const Render::UiLayout &ui_layout )
{
  // the tab bar stretches across its parent window, so give it a child window the size of the layout's ui_tab
  // object. Without one, it stretches across the ##grimoire window.
  const auto tab_it = std::ranges::find( ui_layout.m_tabs, std::string( "tab_bar" ), &Render::UiLayout::Tab::name );
  bool in_child = false;
  if ( tab_it != ui_layout.m_tabs.end() )
  {
    ImGui::SetCursorScreenPos( ImVec2( tab_it->rect.position.x, tab_it->rect.position.y ) );
    // no padding, so the tabs start exactly at the object's position
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.f, 0.f ) );
    constexpr ImGuiWindowFlags kChildFlags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground;
    in_child = ImGui::BeginChild( "##grimoire_tab_bar", ImVec2( tab_it->rect.size.x, tab_it->rect.size.y ), ImGuiChildFlags_None, kChildFlags );
    ImGui::PopStyleVar();
  }
  if ( ( tab_it == ui_layout.m_tabs.end() or in_child ) and ImGui::BeginTabBar( "##grimoire_tabs" ) )
  {
    for ( const auto tab : { Cmp::Grimoire::Tab::NPCS, Cmp::Grimoire::Tab::PLANTS } )
    {
      const ImGuiTabItemFlags flags = m_select_tab and tab == m_tab ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
      if ( ImGui::BeginTabItem( kTabLabels[tab_index( tab )], nullptr, flags ) )
      {
        m_tab = tab;
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
  }
  // EndChild is always paired with BeginChild, whatever BeginChild returned
  if ( tab_it != ui_layout.m_tabs.end() ) { ImGui::EndChild(); }
  m_select_tab = false;
}

template <typename PageT>
void GrimoireOverlaySystem::render_page( const Render::UiLayout &ui_layout, const PageT &page, std::size_t page_count )
{
  render_outlines( ui_layout );
  for ( const auto &icon : ui_layout.m_icons )
  {
    const auto [sub_page, suffix] = resolve_subpage( page, icon.name );
    if ( sub_page and suffix == "icon" ) { render_icon( icon, *sub_page ); }
  }
  render_page_label( ui_layout, page_count );
  for ( const auto &text : ui_layout.m_texts )
  {
    const auto [sub_page, suffix] = resolve_subpage( page, text.name );
    render_text( text, suffix == "name" ? sub_page : nullptr );
  }
  for ( const auto &table : ui_layout.m_tables )
  {
    const auto [sub_page, suffix] = resolve_subpage( page, table.name );
    if ( sub_page and suffix == "actions" ) { render_action_table( table, *sub_page ); }
  }
}

void GrimoireOverlaySystem::render_icon( const Render::UiLayout::Icon &icon, const Cmp::Grimoire::SubPage &sub_page )
{
  Sys::SpriteKey sprite_key = sub_page.sprite_key;
  RenderSystem::safe_render_sprite_screen( sprite_key, { icon.rect.position, Constants::kGridSizePxF }, sub_page.sprite_index,
                                           { icon.scale, icon.scale } );
}

void GrimoireOverlaySystem::render_page_label( const Render::UiLayout &ui_layout, std::size_t page_count )
{
  // Tiled point y is the text baseline; the text's top is about a font size above it, so shift up by font_size so that
  // texts/labels of different font sizes at the same y line up
  for ( const auto &label : ui_layout.m_labels )
  {
    if ( label.name != "page_label" ) { continue; }
    const auto font_size = static_cast<float>( label.font_size );
    draw_text( std::format( "{} of {}", m_page_idx[tab_index( m_tab )] + 1, page_count ), font_size,
               label.rect.position - sf::Vector2f{ 0.f, font_size }, sf::Color::Black );
  }
}

void GrimoireOverlaySystem::render_text( const Render::UiLayout::Text &text, const Cmp::Grimoire::SubPage *sub_page )
{
  const auto font_size = static_cast<float>( text.font_size );
  const sf::Vector2f pos = text.rect.position - sf::Vector2f{ 0.f, font_size };
  if ( sub_page ) { draw_text( Sys::SpriteStore::instance().get( sub_page->sprite_key ).display_name(), font_size, pos, sf::Color::Black ); }
  else { draw_text( text.value, font_size, pos, sf::Color::Black ); }
}

void GrimoireOverlaySystem::render_action_table( const Render::UiLayout::Table &table, const Cmp::Grimoire::SubPage &sub_page )
{
  ImGui::SetCursorScreenPos( ImVec2( table.rect.position.x, table.rect.position.y ) );
  ImGui::PushID( table.name.c_str() );
  // a fixed-size child, so the table is clipped to its layout rect. The wheel turns pages, so it never scrolls.
  constexpr ImGuiWindowFlags kChildFlags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground;
  if ( ImGui::BeginChild( "##table", ImVec2( table.rect.size.x, table.rect.size.y ), ImGuiChildFlags_None, kChildFlags ) )
  {
    // the font is rasterised at kImGuiGrimoireFontPx, so scale it to the layout's font size
    ImGui::SetWindowFontScale( static_cast<float>( table.font_size ) / Utils::kImGuiGrimoireFontPx );
    ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.f, 0.f, 0.f, 1.f ) );
    ImGui::PushStyleColor( ImGuiCol_TableBorderLight, ImVec4( 0.f, 0.f, 0.f, 0.35f ) );
    constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingFixedFit;
    if ( ImGui::BeginTable( "##actions", 2, kTableFlags ) )
    {
      ImGui::TableSetupColumn( "Action", ImGuiTableColumnFlags_WidthFixed );
      ImGui::TableSetupColumn( "Effects", ImGuiTableColumnFlags_WidthStretch );
      for ( const auto &[action_key, caption] : table.rows )
      {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );
        ImGui::TextUnformatted( caption.c_str() );
        ImGui::TableSetColumnIndex( 1 );
        for ( const auto &line : action_effect_lines( sub_page, action_key ) )
        {
          ImGui::TextUnformatted( line.c_str() );
        }
      }
      ImGui::EndTable();
    }
    ImGui::PopStyleColor( 2 );
  }
  ImGui::EndChild();
  ImGui::PopID();
}

std::vector<std::string> GrimoireOverlaySystem::action_effect_lines( const Cmp::Grimoire::SubPage &sub_page, const std::string &action_key )
{
  const auto action_it = sub_page.action_map.find( action_key );
  if ( action_it == sub_page.action_map.end() ) { return { "?????" }; }
  // reward, stats then side effects, or "none" if the action is discovered but has no effect
  std::vector<std::string> lines;
  const Cmp::BaseAction &action = action_it->second;
  const bool has_reward = append_reward_lines( lines, action );
  const bool has_stats = append_stat_lines( lines, action );
  const bool has_side_effects = append_side_effect_lines( lines, action );
  if ( not has_reward and not has_stats and not has_side_effects ) { lines.emplace_back( "none" ); }
  return lines;
}

bool GrimoireOverlaySystem::append_reward_lines( std::vector<std::string> &lines, const Cmp::BaseAction &action )
{
  // the reward goes first so it sits on the action's caption row, e.g. "Offering | (Elder Flute)"
  const auto reward = action.reward();
  if ( not reward ) { return false; }
  lines.push_back( std::format( "({})", reward_display_name( *reward ) ) );
  return true;
}

bool GrimoireOverlaySystem::append_stat_lines( std::vector<std::string> &lines, const Cmp::BaseAction &action )
{
  // one line per non-zero stat
  const std::string tick_suffix = action.interval() > 0.f ? std::format( "pt/{:g}s", action.interval() ) : "";
  bool has_effect = false;
  const auto draw_stat = [&]( std::string_view stat_name, int value )
  {
    if ( value == 0 ) { return; }
    lines.push_back( std::format( "{}: {}{}", stat_name, value, tick_suffix ) );
    has_effect = true;
  };
  draw_stat( "health", action.health() );
  draw_stat( "fear", action.fear() );
  draw_stat( "despair", action.despair() );
  draw_stat( "infamy", action.infamy() );
  draw_stat( "luck", action.luck() );
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

bool GrimoireOverlaySystem::append_side_effect_lines( std::vector<std::string> &lines, const Cmp::BaseAction &action )
{
  // side effects, with their chance of firing
  const Cmp::SideEffects &effects = action.side_effects();
  const int player_luck = Utils::Player::get_stats( reg() ).luck();
  const auto draw_effect = [&]( std::string_view label, float chance ) { lines.push_back( std::format( "{}: {:.0f}%", label, chance * 100.f ) ); };
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
  if ( effects.damage_npc )
  {
    // always applies, so there's no chance to show
    for ( const auto &entry : effects.damage_npc->entries )
    {
      lines.push_back( std::format( "damages {}: -{:g}/{:g}s", npc_display_name( entry.npc_type.str() ), entry.amount, entry.tick ) );
    }
  }
  return not effects.empty();
}
} // namespace Game::Sys
