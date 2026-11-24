#include <Components/Cart/Barrel.hpp>
#include <Components/Cart/BarrelOverlayOpen.hpp>
#include <Components/Cart/MultiBlock.hpp>
#include <Systems/Render/CartOverlaySystem.hpp>

#include <algorithm>
#include <imgui.h>
#include <string>

namespace Game::Sys
{

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

  auto barrel_view = reg().view<Cmp::Cart::Barrel>();
  if ( barrel_view.empty() ) { return; }

  // ImGui draws in window pixels, so follow the cart's centre through the world view as the camera moves
  const sf::Vector2f cart_center( m_window.mapCoordsToPixel( cart_view.get<Cmp::Cart::MultiBlock>( cart_entt ).getCenter(), get_world_view() ) );
  const float scale = Render::UiLayout::ui_scale( m_window.getSize() );
  const sf::Vector2f slot_size = m_slot_ui_layout->design_size() * scale;
  const float row_width = slot_size.x * static_cast<float>( barrel_view.size() );
  sf::Vector2f slot_origin( cart_center.x - ( row_width / 2.f ), cart_center.y - ( slot_size.y / 2.f ) );

  // the panel is authored around one slot: keep that padding around the whole row. Set every frame, as sync_to_scale()
  // only restores the authored size when the scale or origin changes.
  const sf::Vector2f panel_pad = ( m_panel_ui_layout->design_size() - m_slot_ui_layout->design_size() ) * scale / 2.f;
  m_panel_ui_layout->sync_to_scale( scale, slot_origin - panel_pad );
  const auto panel_it = std::ranges::find( m_panel_ui_layout->m_outlines, std::string( kPanelOutlineName ), &Render::UiLayout::Outline::name );
  if ( panel_it != m_panel_ui_layout->m_outlines.end() )
  {
    panel_it->rect.size = { row_width + ( 2.f * panel_pad.x ), slot_size.y + ( 2.f * panel_pad.y ) };
  }

  begin_overlay_window( "##cart_barrels" );
  render_outlines_imgui( *m_panel_ui_layout );
  // same view order as CartHorseSystem::transfer_selected_item(), so barrels fill from the left
  for ( auto [barrel_entt, barrel_cmp] : barrel_view.each() )
  {
    m_slot_ui_layout->sync_to_scale( scale, slot_origin );
    render_outlines_imgui( *m_slot_ui_layout );
    for ( const auto &icon : m_slot_ui_layout->m_icons )
    {
      // the icon's name is its part's sprite; its sprite_type is only the placeholder shown in Tiled
      const Sys::SpriteKey part_key( icon.name );
      const bool *filled = barrel_cmp.part( part_key );
      if ( filled and *filled ) { render_icon( part_key, 0, icon.rect.position, icon.scale ); }
    }
    slot_origin.x += slot_size.x;
  }
  end_overlay_window();
}

} // namespace Game::Sys
