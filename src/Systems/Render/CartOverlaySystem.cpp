#include <Components/Cart/Barrel.hpp>
#include <Components/Cart/BarrelOverlayOpen.hpp>
#include <Components/Cart/MultiBlock.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/Render/CartOverlaySystem.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/Constants.hpp>

#include <algorithm>
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
} // namespace

CartOverlaySystem::CartOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseOverlaySystem( reg, window, sound_bank ),
      m_slot_ui_layout( std::make_unique<Render::UiLayout>( "res/ui/barrel_slot.json" ) ),
      m_panel_ui_layout( std::make_unique<Render::UiLayout>( "res/ui/barrel_panel.json" ) )
{
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
                                            std::span<const IconRow> icon_rows, sf::Vector2f top_center )
{
  const float scale = Render::UiLayout::ui_scale( m_window.getSize() );
  constexpr float kSectionGap = 40.f;

  // this scene has no OverlayArbitrationSystem frame, so pump our own
  ImGui::SFML::Update( m_window, std::max( dt, sf::microseconds( 1 ) ) );
  begin_overlay_window( "##level_tally" );

  sf::Vector2f section_top = top_center;
  if ( not barrels.empty() )
  {
    section_top.y += render_caption( "Cart", section_top, scale );
    const float panel_height = m_panel_ui_layout->design_size().y * scale;
    const float row_height = render_barrel_row( barrels, { section_top.x, section_top.y + ( panel_height / 2.f ) }, barrel_progress );
    section_top.y += row_height + ( kSectionGap * scale );
  }

  const bool any_icons = std::ranges::any_of( icon_rows, []( const IconRow &row ) { return not row.sprites.empty(); } );
  if ( any_icons )
  {
    section_top.y += render_caption( "Inventory", section_top, scale );
    std::ignore = render_icon_rows( icon_rows, section_top );
  }

  end_overlay_window();
  ImGui::SFML::Render( m_window );
}

float CartOverlaySystem::render_icon_rows( std::span<const IconRow> icon_rows, sf::Vector2f top_center )
{
  const float scale = Render::UiLayout::ui_scale( m_window.getSize() );
  constexpr float kRowGap = 20.f;
  constexpr float kIconScale = 2.f;
  const float icon_scale = kIconScale * scale;
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
  render_panel( sf::FloatRect( { top_center.x - ( panel_size.x / 2.f ), top_center.y }, panel_size ), scale );

  float row_top = top_center.y + panel_pad.y;
  for ( std::size_t row_idx = 0; row_idx < icon_rows.size(); ++row_idx )
  {
    const IconRow &row = icon_rows[row_idx];
    const RowSize &row_size = row_sizes.at( row_idx );
    if ( row.sprites.empty() ) { continue; }

    float icon_x = top_center.x - ( row_size.width / 2.f );
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

void CartOverlaySystem::render_panel( const sf::FloatRect &rect, float scale )
{
  // the panel is authored at (0, 0), so this places it at the rectangle (and scales its border); then fit its size
  m_panel_ui_layout->sync_to_scale( scale, rect.position );
  const auto panel_it = std::ranges::find( m_panel_ui_layout->m_outlines, std::string( kPanelOutlineName ), &Render::UiLayout::Outline::name );
  if ( panel_it != m_panel_ui_layout->m_outlines.end() ) { panel_it->rect = rect; }
  render_outlines_imgui( *m_panel_ui_layout );
}

float CartOverlaySystem::render_caption( std::string_view caption, sf::Vector2f top_center, float scale )
{
  constexpr float kCaptionFontSize = 32.f;
  constexpr float kCaptionGap = 10.f;
  const float font_size = kCaptionFontSize * scale;
  draw_text( caption, font_size, top_center, sf::Color::White, TextAlign::CENTER, sf::Color::Black, 2.f * scale );
  return measure_text( caption, font_size ).y + ( kCaptionGap * scale );
}

float CartOverlaySystem::render_barrel_row( std::span<const Cmp::Cart::Barrel> barrels, sf::Vector2f row_center,
                                            std::optional<BarrelProgress> progress )
{
  const float scale = Render::UiLayout::ui_scale( m_window.getSize() );
  const sf::Vector2f slot_size = m_slot_ui_layout->design_size() * scale;
  const float row_width = slot_size.x * static_cast<float>( barrels.size() );
  sf::Vector2f slot_origin( row_center.x - ( row_width / 2.f ), row_center.y - ( slot_size.y / 2.f ) );

  // the panel is authored around one slot: keep that padding around the whole row
  const sf::Vector2f panel_pad = ( m_panel_ui_layout->design_size() - m_slot_ui_layout->design_size() ) * scale / 2.f;
  const sf::Vector2f panel_size{ row_width + ( 2.f * panel_pad.x ), slot_size.y + ( 2.f * panel_pad.y ) };
  render_panel( sf::FloatRect( slot_origin - panel_pad, panel_size ), scale );
  for ( std::size_t barrel_idx = 0; barrel_idx < barrels.size(); ++barrel_idx )
  {
    const Cmp::Cart::Barrel &barrel_cmp = barrels[barrel_idx];
    m_slot_ui_layout->sync_to_scale( scale, slot_origin );
    render_outlines_imgui( *m_slot_ui_layout );
    for ( const auto &icon : m_slot_ui_layout->m_icons )
    {
      // the icon's name is its part's sprite; its sprite_type is only the placeholder shown in Tiled
      const Sys::SpriteKey part_key( icon.name );
      const bool *filled = barrel_cmp.part( part_key );
      if ( not filled or not *filled ) { continue; }
      render_icon( part_key, 0, icon.rect.position, icon.scale );

      if ( not progress ) { continue; }
      const auto part_it = std::ranges::find( Cmp::Cart::Barrel::kParts, std::string_view( icon.name ),
                                              &std::pair<std::string_view, bool Cmp::Cart::Barrel::*>::first );
      const auto part_idx = static_cast<std::size_t>( std::distance( Cmp::Cart::Barrel::kParts.begin(), part_it ) );
      const sf::FloatRect rect( icon.rect.position, icon_size( part_key, icon.scale ) );
      const bool counted = barrel_idx < progress->barrel or ( barrel_idx == progress->barrel and ( progress->whole_barrel or part_idx < progress->part ) );
      if ( counted ) { dim_rect( rect ); }
      else if ( barrel_idx == progress->barrel and part_idx == progress->part ) { highlight_rect( rect, scale ); }
    }

    if ( progress and progress->whole_barrel and barrel_idx == progress->barrel ) { highlight_rect( sf::FloatRect( slot_origin, slot_size ), scale ); }
    slot_origin.x += slot_size.x;
  }
  return panel_size.y;
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
