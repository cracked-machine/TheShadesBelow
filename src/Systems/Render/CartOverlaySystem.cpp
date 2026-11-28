#include <Components/Cart/Barrel.hpp>
#include <Components/Cart/BarrelOverlayOpen.hpp>
#include <Components/Cart/MultiBlock.hpp>
#include <Components/Persistent/MaxLevelDepth.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Player/Wealth.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/CartOverlaySystem.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/Constants.hpp>
#include <Utils/ImGuiTheme.hpp>
#include <Utils/Player.hpp>

#include <algorithm>
#include <format>
#include <stdexcept>
#include <imgui-SFML.h>
#include <imgui.h>
#include <string>
#include <vector>

namespace Game::Sys
{

namespace
{
//! @brief Window pixel size of a sprite drawn by render_icon() at `scale`.
sf::Vector2f icon_size( const Sys::SpriteKey &sprite, float scale )
{
  try
  {
    return Sys::SpriteStore::instance().get( sprite ).sprite_size() * scale;
  } catch ( const std::out_of_range & )
  {
    return Constants::kGridSizePxF * scale;
  }
}

//! @brief The level complete layout file.
constexpr const char *kLevelCompleteLayout = "res/ui/levelcomplete.json";

//! @brief Find a level complete layout element by name.
//! @throws std::runtime_error if there is none, naming it.
template <typename Element>
const Element &find_element( const std::vector<Element> &elements, std::string_view name )
{
  const auto it = std::ranges::find( elements, name, &Element::name );
  if ( it == elements.end() ) { throw std::runtime_error( std::format( "Missing '{}' element in {}", name, kLevelCompleteLayout ) ); }
  return *it;
}

//! @brief Top-left of label text drawn at a layout label's position. Tiled point y is the text baseline; the text's top
//! is about a font size above it (see GrimoireOverlaySystem::render_page_label()).
sf::Vector2f label_text_pos( const Render::UiLayout::Label &label )
{
  return label.rect.position - sf::Vector2f{ 0.f, static_cast<float>( label.font_size ) };
}
} // namespace

CartOverlaySystem::CartOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseOverlaySystem( reg, window, sound_bank ),
      m_slot_ui_layout( std::make_unique<Render::UiLayout>( "res/ui/barrel_slot.json" ) ),
      m_panel_ui_layout( std::make_unique<Render::UiLayout>( "res/ui/barrel_panel.json" ) ),
      m_level_complete_ui_layout( std::make_unique<Render::UiLayout>( kLevelCompleteLayout ) )
{
  // check up front, so a broken layout fails at startup rather than on reaching the level complete screen
  const auto &layout = *m_level_complete_ui_layout;
  for ( const char *name : { "title_lbl", "continue_txt", "wealth_txt" } )
  {
    std::ignore = find_element( layout.m_labels, name );
  }
  for ( const char *name : { "table_icon", "cart_icon", "inventory_icon" } )
  {
    std::ignore = find_element( layout.m_icons, name );
  }
  for ( const auto &[key, caption] : find_element( layout.m_tables, "tally_table" ).rows )
  {
    if ( not tally_value( VictoryTally{}, key ) )
    {
      throw std::runtime_error( std::format( "Unknown 'tally_table' row key '{}' in {}", key, kLevelCompleteLayout ) );
    }
  }
}

void CartOverlaySystem::update( [[maybe_unused]] sf::Time dt )
{
  auto cart_view = reg().view<Cmp::Cart::MultiBlock, Cmp::Cart::BarrelOverlayOpen>();
  const auto cart_entt = cart_view.front();
  if ( cart_entt == entt::null ) { return; }

  // same view order as CartHorseSystem::transfer_selected_item(), so barrels fill from the left
  std::vector<Cmp::Cart::Barrel> barrels;
  for ( auto [barrel_entt, barrel_cmp] : reg().view<Cmp::Cart::Barrel>().each() )
  {
    barrels.push_back( barrel_cmp );
  }
  if ( barrels.empty() ) { return; }

  // ImGui draws in window pixels, so follow the cart's centre through the world view as the camera moves
  const sf::Vector2f cart_center( m_window.mapCoordsToPixel( cart_view.get<Cmp::Cart::MultiBlock>( cart_entt ).getCenter(), get_world_view() ) );

  begin_overlay_window( "##cart_barrels" );
  std::ignore = render_barrel_row( barrels, cart_center, std::nullopt );
  end_overlay_window();
}

void CartOverlaySystem::render_level_tally( sf::Time dt, std::span<const Cmp::Cart::Barrel> barrels, std::optional<BarrelProgress> barrel_progress,
                                            std::span<const IconRow> icon_rows, bool show_continue, const VictoryTally &tally,
                                            const std::optional<Sys::SpriteKey> &table_item, sf::Color table_highlight )
{
  const float scale = Render::UiLayout::ui_scale( m_window.getSize() );
  m_level_complete_ui_layout->sync_to( m_window.getSize() );
  const auto &layout = *m_level_complete_ui_layout;

  // this scene has no OverlayArbitrationSystem frame, so pump our own
  ImGui::SFML::Update( m_window, std::max( dt, sf::microseconds( 1 ) ) );
  begin_overlay_window( "##level_tally" );

  render_level_labels( show_continue );
  render_tally_table( tally );
  if ( table_item ) { render_table_item( *table_item, table_highlight ); }

  // each section hangs from the top-left of its layout rect and grows rightwards
  if ( not barrels.empty() )
  {
    sf::Vector2f section_top_left = find_element( layout.m_icons, "cart_icon" ).rect.position;
    section_top_left.y += render_caption( "Cart", section_top_left, scale );
    const sf::Vector2f panel_size = barrel_row_panel_size( barrels.size(), scale );
    std::ignore = render_barrel_row( barrels, section_top_left + ( panel_size / 2.f ), barrel_progress );
  }

  // the inventory section is always shown: with nothing to count it shows the player's empty slots instead
  {
    sf::Vector2f section_top_left = find_element( layout.m_icons, "inventory_icon" ).rect.position;
    section_top_left.y += render_caption( "Inventory", section_top_left, scale );
    const bool any_icons = std::ranges::any_of( icon_rows, []( const IconRow &row ) { return not row.sprites.empty(); } );
    if ( any_icons ) { std::ignore = render_icon_rows( icon_rows, section_top_left ); }
    else { std::ignore = render_empty_slots( Utils::Player::get_inventory_slot_count( reg() ), section_top_left ); }
  }

  end_overlay_window();
  ImGui::SFML::Render( m_window );
}

void CartOverlaySystem::render_level_labels( bool show_continue )
{
  const auto &layout = *m_level_complete_ui_layout;
  const float outline = 2.f * layout.scale();
  const auto draw_label = [outline]( const Render::UiLayout::Label &label, std::string_view text )
  { draw_text( text, static_cast<float>( label.font_size ), label_text_pos( label ), sf::Color::White, to_text_align( label.align ), sf::Color::Black, outline ); };

  const int max_level_depth = Sys::PersistSystem::get<Cmp::Persist::MaxLevelDepth>( reg() ).get_value();
  const unsigned int depth = max_level_depth - Utils::Player::get_level_depth( reg() ).get_count();
  const bool final_depth = Utils::Player::is_final_level_depth( reg() );

  draw_label( find_element( layout.m_labels, "title_lbl" ),
              final_depth ? std::format( "Final Level {} Complete!", depth ) : std::format( "Level {} Complete!", depth ) );
  if ( show_continue )
  {
    draw_label( find_element( layout.m_labels, "continue_txt" ), final_depth ? "Press <R> key to return to the title" : "Press <R> key to continue" );
  }
  draw_label( find_element( layout.m_labels, "wealth_txt" ), std::format( "Player wealth: {}", Utils::Player::get_wealth( reg() ).wealth ) );
}

void CartOverlaySystem::render_tally_table( const VictoryTally &tally )
{
  const auto &table = find_element( m_level_complete_ui_layout->m_tables, "tally_table" );
  ImGui::SetCursorScreenPos( ImVec2( table.rect.position.x, table.rect.position.y ) );
  ImGui::PushID( table.name.c_str() );
  // the font is rasterised at kImGuiGrimoireFontPx, so scale it to the layout's (already UI-scaled) font size
  ImGui::SetWindowFontScale( static_cast<float>( table.font_size ) / Utils::kImGuiGrimoireFontPx );
  ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.f, 1.f, 1.f, 1.f ) );
  constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_SizingFixedFit;
  if ( ImGui::BeginTable( "##tally", 2, kTableFlags ) )
  {
    ImGui::TableSetupColumn( "Caption", ImGuiTableColumnFlags_WidthFixed );
    ImGui::TableSetupColumn( "Value", ImGuiTableColumnFlags_WidthFixed );
    for ( const auto &[key, caption] : table.rows )
    {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex( 0 );
      ImGui::TextUnformatted( caption.c_str() );
      ImGui::TableSetColumnIndex( 1 );
      // row keys are checked in the constructor
      ImGui::TextUnformatted( std::to_string( tally_value( tally, key ).value_or( 0 ) ).c_str() );
    }
    ImGui::EndTable();
  }
  ImGui::PopStyleColor();
  ImGui::SetWindowFontScale( 1.f );
  ImGui::PopID();
}

void CartOverlaySystem::render_table_item( const Sys::SpriteKey &item, sf::Color highlight )
{
  const auto &icon = find_element( m_level_complete_ui_layout->m_icons, "table_icon" );
  // fit the sprite inside the rect, centred
  const sf::Vector2f sprite_size = icon_size( item, 1.f );
  const float scale = std::min( icon.rect.size.x / sprite_size.x, icon.rect.size.y / sprite_size.y );
  const sf::Vector2f size = sprite_size * scale;
  const sf::Vector2f pos = icon.rect.getCenter() - ( size / 2.f );
  render_icon( item, 0, pos, scale );
  highlight_rect( sf::FloatRect( pos, size ), m_level_complete_ui_layout->scale(), highlight );
}

std::optional<int> CartOverlaySystem::tally_value( const VictoryTally &tally, std::string_view key )
{
  if ( key == "score_wealth" ) { return tally.score_wealth; }
  if ( key == "parts" ) { return tally.parts; }
  if ( key == "full_barrels" ) { return tally.full_barrels; }
  if ( key == "cats" ) { return tally.cats; }
  if ( key == "spare_parts" ) { return tally.spare_parts; }
  return std::nullopt;
}

float CartOverlaySystem::render_icon_rows( std::span<const IconRow> icon_rows, sf::Vector2f top_left )
{
  const float scale = Render::UiLayout::ui_scale( m_window.getSize() );
  constexpr float kRowGap = 20.f;
  const float icon_scale = kInventoryIconScale * scale;
  // same padding round the rows as the barrel panel keeps round its slots
  const sf::Vector2f panel_pad = ( m_panel_ui_layout->design_size() - m_slot_ui_layout->design_size() ) * scale / 2.f;

  // measure every row first, to size the panel round them
  struct RowSize
  {
    float width{ 0.f };
    float height{ 0.f };
    float gap{ 0.f };
  };
  std::vector<RowSize> row_sizes;
  float content_width = 0.f;
  float content_height = 0.f;
  for ( const IconRow &row : icon_rows )
  {
    RowSize row_size;
    for ( const Sys::SpriteKey &sprite : row.sprites )
    {
      const sf::Vector2f size = icon_size( sprite, icon_scale );
      row_size.gap = std::max( row_size.gap, size.x / 4.f );
      row_size.width += size.x;
      row_size.height = std::max( row_size.height, size.y );
    }
    if ( not row.sprites.empty() )
    {
      row_size.width += static_cast<float>( row.sprites.size() - 1 ) * row_size.gap;
      content_width = std::max( content_width, row_size.width );
      content_height += row_size.height + ( content_height > 0.f ? kRowGap * scale : 0.f );
    }
    row_sizes.push_back( row_size );
  }

  const sf::Vector2f panel_size( content_width + ( 2.f * panel_pad.x ), content_height + ( 2.f * panel_pad.y ) );
  render_panel( sf::FloatRect( top_left, panel_size ), scale );

  float row_top = top_left.y + panel_pad.y;
  for ( std::size_t row_idx = 0; row_idx < icon_rows.size(); ++row_idx )
  {
    const IconRow &row = icon_rows[row_idx];
    const RowSize &row_size = row_sizes.at( row_idx );
    if ( row.sprites.empty() ) { continue; }

    float icon_x = top_left.x + panel_pad.x;
    for ( std::size_t idx = 0; idx < row.sprites.size(); ++idx )
    {
      const Sys::SpriteKey &sprite = row.sprites[idx];
      const sf::Vector2f size = icon_size( sprite, icon_scale );
      const sf::Vector2f icon_pos( icon_x, row_top + ( ( row_size.height - size.y ) / 2.f ) );
      render_icon( sprite, 0, icon_pos, icon_scale );
      if ( row.progress )
      {
        const sf::FloatRect rect( icon_pos, size );
        if ( idx < row.progress->current ) { dim_rect( rect ); }
        else if ( idx == row.progress->current ) { highlight_rect( rect, scale, row.highlight ); }
      }
      icon_x += size.x + row_size.gap;
    }
    row_top += row_size.height + ( kRowGap * scale );
  }
  return panel_size.y;
}

float CartOverlaySystem::render_empty_slots( std::size_t slot_count, sf::Vector2f top_left )
{
  if ( slot_count == 0 ) { return 0.f; }
  const float scale = Render::UiLayout::ui_scale( m_window.getSize() );
  // the same cell size and spacing as an icon in render_icon_rows()
  const sf::Vector2f slot_size = Constants::kGridSizePxF * kInventoryIconScale * scale;
  const float gap = slot_size.x / 4.f;
  const sf::Vector2f panel_pad = ( m_panel_ui_layout->design_size() - m_slot_ui_layout->design_size() ) * scale / 2.f;
  const sf::Vector2f panel_size( ( static_cast<float>( slot_count ) * slot_size.x ) + ( static_cast<float>( slot_count - 1 ) * gap ) + ( 2.f * panel_pad.x ),
                                 slot_size.y + ( 2.f * panel_pad.y ) );
  render_panel( sf::FloatRect( top_left, panel_size ), scale );

  // styled like the barrel slot's empty part outlines
  sf::Color fill_color( 0x40, 0x40, 0x40, 0x40 );
  sf::Color line_color = sf::Color::Black;
  if ( not m_slot_ui_layout->m_outlines.empty() )
  {
    fill_color = m_slot_ui_layout->m_outlines.front().fill_color;
    line_color = m_slot_ui_layout->m_outlines.front().line_color;
  }
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  sf::Vector2f slot_pos = top_left + panel_pad;
  for ( std::size_t idx = 0; idx < slot_count; ++idx )
  {
    const ImVec2 slot_min( slot_pos.x, slot_pos.y );
    const ImVec2 slot_max( slot_pos.x + slot_size.x, slot_pos.y + slot_size.y );
    draw_list->AddRectFilled( slot_min, slot_max, IM_COL32( fill_color.r, fill_color.g, fill_color.b, fill_color.a ) );
    draw_list->AddRect( slot_min, slot_max, IM_COL32( line_color.r, line_color.g, line_color.b, line_color.a ), 0.f, 0, 3.f * scale );
    slot_pos.x += slot_size.x + gap;
  }
  return panel_size.y;
}

void CartOverlaySystem::render_panel( const sf::FloatRect &rect, float scale )
{
  // the panel is authored at (0, 0), so this places it at the rectangle (and scales its border); then fit its size
  m_panel_ui_layout->sync_to_scale( scale, rect.position );
  const auto panel_it = std::ranges::find( m_panel_ui_layout->m_outlines, std::string( kPanelOutlineName ), &Render::UiLayout::Outline::name );
  if ( panel_it != m_panel_ui_layout->m_outlines.end() ) { panel_it->rect = rect; }
  render_outlines_imgui( *m_panel_ui_layout );
}

float CartOverlaySystem::render_caption( std::string_view caption, sf::Vector2f top_left, float scale )
{
  constexpr float kCaptionFontSize = 32.f;
  constexpr float kCaptionGap = 10.f;
  const float font_size = kCaptionFontSize * scale;
  draw_text( caption, font_size, top_left, sf::Color::White, TextAlign::LEFT, sf::Color::Black, 2.f * scale );
  return measure_text( caption, font_size ).y + ( kCaptionGap * scale );
}

float CartOverlaySystem::render_barrel_row( std::span<const Cmp::Cart::Barrel> barrels, sf::Vector2f row_center,
                                            std::optional<BarrelProgress> progress )
{
  const float scale = Render::UiLayout::ui_scale( m_window.getSize() );
  const sf::Vector2f slot_size = m_slot_ui_layout->design_size() * scale;
  const sf::Vector2f panel_pad = ( m_panel_ui_layout->design_size() - m_slot_ui_layout->design_size() ) * scale / 2.f;
  const sf::Vector2f panel_size = barrel_row_panel_size( barrels.size(), scale );
  const sf::Vector2f panel_pos = row_center - ( panel_size / 2.f );
  render_panel( sf::FloatRect( panel_pos, panel_size ), scale );

  sf::Vector2f slot_origin = panel_pos + panel_pad;
  for ( std::size_t barrel_idx = 0; barrel_idx < barrels.size(); ++barrel_idx )
  {
    render_barrel_slot( barrels[barrel_idx], barrel_idx, slot_origin, scale, progress );
    slot_origin.x += slot_size.x;
  }
  return panel_size.y;
}

sf::Vector2f CartOverlaySystem::barrel_row_panel_size( std::size_t barrel_count, float scale ) const
{
  const sf::Vector2f slot_size = m_slot_ui_layout->design_size() * scale;
  // the panel is authored around one slot: keep that padding around the whole row
  const sf::Vector2f panel_pad = ( m_panel_ui_layout->design_size() - m_slot_ui_layout->design_size() ) * scale / 2.f;
  return { ( slot_size.x * static_cast<float>( barrel_count ) ) + ( 2.f * panel_pad.x ), slot_size.y + ( 2.f * panel_pad.y ) };
}

void CartOverlaySystem::render_barrel_slot( const Cmp::Cart::Barrel &barrel_cmp, std::size_t barrel_idx, sf::Vector2f slot_origin,
                                            float scale, std::optional<BarrelProgress> progress )
{
  m_slot_ui_layout->sync_to_scale( scale, slot_origin );
  render_outlines_imgui( *m_slot_ui_layout );
  for ( const auto &icon : m_slot_ui_layout->m_icons )
  {
    // the icon's name is its part's sprite; its sprite_type is only the placeholder shown in Tiled
    const Sys::SpriteKey part_key( icon.name );
    const bool *filled = barrel_cmp.part( part_key );
    if ( not filled or not *filled ) { continue; }

    bool counted = false;
    bool current = false;
    if ( progress )
    {
      const auto part_it = std::ranges::find( Cmp::Cart::Barrel::kParts, std::string_view( icon.name ),
                                              &std::pair<std::string_view, bool Cmp::Cart::Barrel::*>::first );
      const auto part_idx = static_cast<std::size_t>( std::distance( Cmp::Cart::Barrel::kParts.begin(), part_it ) );
      counted = barrel_idx < progress->barrel or ( barrel_idx == progress->barrel and ( progress->whole_barrel or part_idx < progress->part ) );
      current = not counted and barrel_idx == progress->barrel and part_idx == progress->part;
    }

    render_icon( part_key, 0, icon.rect.position, icon.scale );
    const sf::FloatRect rect( icon.rect.position, icon_size( part_key, icon.scale ) );
    if ( counted ) { dim_rect( rect ); }
    else if ( current ) { highlight_rect( rect, scale ); }
  }

  if ( progress and progress->whole_barrel and barrel_idx == progress->barrel )
  {
    highlight_rect( sf::FloatRect( slot_origin, m_slot_ui_layout->design_size() * scale ), scale );
  }
}

void CartOverlaySystem::dim_rect( const sf::FloatRect &rect )
{
  ImGui::GetWindowDrawList()->AddRectFilled( ImVec2( rect.position.x, rect.position.y ),
                                             ImVec2( rect.position.x + rect.size.x, rect.position.y + rect.size.y ), IM_COL32( 0, 0, 0, 140 ) );
}

void CartOverlaySystem::highlight_rect( const sf::FloatRect &rect, float scale, sf::Color colour )
{
  const ImVec2 top_left( rect.position.x, rect.position.y );
  const ImVec2 bottom_right( rect.position.x + rect.size.x, rect.position.y + rect.size.y );
  ImGui::GetWindowDrawList()->AddRectFilled( top_left, bottom_right, IM_COL32( colour.r, colour.g, colour.b, 70 ) );
  ImGui::GetWindowDrawList()->AddRect( top_left, bottom_right, IM_COL32( colour.r, colour.g, colour.b, 255 ), 0.f, 0, 3.f * scale );
}

} // namespace Game::Sys
