#include <Components/Particle/SpriteOwner.hpp>
#include <Components/Position.hpp>
#include <Components/Shop/Inventory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Systems/Render/ShopOverlaySystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/Constants.hpp>

#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include <ranges>
#include <string>

namespace Game::Sys
{

ShopOverlaySystem::ShopOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseOverlaySystem( reg, window, sound_bank ),
      m_shop_ui_layout( std::make_unique<Render::UiLayout>( "res/ui/shop.json" ) )
{
}

void ShopOverlaySystem::update( [[maybe_unused]] sf::Time dt )
{
  // only draw the shop if its enabled
  auto inventory_view = reg().view<Cmp::Shop::Inventory>();
  if ( inventory_view.empty() ) { return; }
  auto &inventory_cmp = inventory_view.get<Cmp::Shop::Inventory>( inventory_view.front() );
  if ( not inventory_cmp.is_enabled ) return;

  if ( not m_shop_ui_layout )
  {
    SPDLOG_CRITICAL( "UiLayout object is not initialised. Cannot draw value overlay" );
    return;
  }
  // the layout is authored at its Tiled map size; rescale it if the window resolution has changed (no-op otherwise)
  m_shop_ui_layout->sync_to( m_window.getSize() );

  // Draw all UI outlines
  render_outlines( *m_shop_ui_layout );

  // The shop flames are an unassigned pool (see Factory::Particle::sync_flames_for_shop_inventory):
  // each torch slot just takes the next unused one.
  auto flame_view = reg().view<Cmp::Particle::SpriteOwner>();
  auto flame_it = flame_view.begin();
  auto next_slot_flame = [&]() -> Cmp::Particle::SpriteOwner *
  {
    while ( flame_it != flame_view.end() )
    {
      auto &owner = flame_view.get<Cmp::Particle::SpriteOwner>( *flame_it );
      ++flame_it;
      if ( owner.sprite and owner.sprite->get_tag() == Factory::Particle::kShopSlotFlameTag ) return &owner;
    }
    return nullptr;
  };

  // Draw all UI Icons
  for ( auto [icon, slot] : std::views::zip( m_shop_ui_layout->m_icons, inventory_cmp.m_slots ) )
  {
    auto &[item, price] = slot;
    const auto &world_item = Sys::ItemStore::instance().get( item );
    const auto &sprite_type = world_item.sprite_type;
    // Use the default scale/size unless its a plant then its need to be resized/repositioned to fit in the UI box
    const float shop_scale = m_shop_ui_layout->scale();
    auto sprite_scale = sf::Vector2f{ icon.scale, icon.scale };
    auto sprite_pos = Cmp::Position( icon.rect.position, Constants::kGridSizePxF );
    if ( sprite_type.contains( "plant" ) )
    {
      // one design-space step shorter, i.e. (sprite_scale - 1) before the UI scale was applied
      sprite_scale = sf::Vector2f{ icon.scale, icon.scale - shop_scale };
      sprite_pos = Cmp::Position( { icon.rect.position.x, icon.rect.position.y - ( 8.f * shop_scale ) }, Constants::kGridSizePxF );
    }

    RenderSystem::safe_render_sprite_screen( sprite_type, sprite_pos, world_item.icon_sprite_index(), sprite_scale );

    if ( item != "item.torch" ) continue;
    auto *flame_owner = next_slot_flame();
    if ( not flame_owner ) continue;
    auto &flame = *flame_owner->sprite;
    // same wick offset as the player inventory icon in HudOverlaySystem::render_ui_particles()
    const sf::Vector2f emitter_pos{ icon.rect.position.x + ( icon.scale * 8.f ), icon.rect.position.y + ( icon.scale * 6.f ) };
    if ( flame.get_emitter_position() != emitter_pos )
    {
      // slots shift when an item is bought: drop the particles still rising from the old position
      flame.set_emitter_position( emitter_pos );
      flame.clear();
    }
    flame.set_view_transform( m_window, m_window.getDefaultView() );
    flame.restart();
    draw_screen( flame );
  }

  //! @brief Helper to draw predefined `sf_text` at `pos`
  auto draw_label_text_at = [&]( sf::Text &sf_text, const sf::Vector2f &pos, const std::string &align = "left" )
  {
    if ( align == "center" ) { sf_text.setPosition( { pos.x - sf_text.getLocalBounds().getCenter().x, pos.y } ); }
    else if ( align == "left" ) { sf_text.setPosition( pos ); }
    else if ( align == "right" ) { sf_text.setPosition( { pos.x - sf_text.getLocalBounds().size.x, pos.y } ); }

    draw_screen( sf_text );
  };

  // Draw all text labels (index, description, price)
  for ( auto [i, slot] : std::views::enumerate( inventory_cmp.m_slots ) )
  {
    auto &[item, price] = slot;

    const unsigned int slot_char_size = scaled_char_size( 30, m_shop_ui_layout.get() );
    sf::Text slot_idx_txt( m_font, std::to_string( i + 1 ), slot_char_size );
    slot_idx_txt.setFillColor( sf::Color::Black );

    Sys::SpriteKey sprite_mtype = Sys::ItemStore::instance().get( item ).sprite_type;
    sf::Text slot_desc_txt( m_font, Sys::SpriteStore::instance().get( sprite_mtype ).display_name(), slot_char_size );
    slot_desc_txt.setFillColor( sf::Color::Black );

    sf::Text slot_price_txt( m_font, std::to_string( price ), slot_char_size );
    slot_price_txt.setFillColor( sf::Color::Black );

    const std::string idx_key = "slot" + std::to_string( i + 1 ) + "_idx";
    const std::string desc_key = "slot" + std::to_string( i + 1 ) + "_desc";
    const std::string price_key = "slot" + std::to_string( i + 1 ) + "_price";
    for ( const auto &ui_label : m_shop_ui_layout->m_labels )
    {
      if ( ui_label.name.contains( idx_key ) ) { draw_label_text_at( slot_idx_txt, ui_label.rect.position, ui_label.align ); }
      else if ( ui_label.name.contains( desc_key ) ) { draw_label_text_at( slot_desc_txt, ui_label.rect.position, ui_label.align ); }
      else if ( ui_label.name.contains( price_key ) ) { draw_label_text_at( slot_price_txt, ui_label.rect.position, ui_label.align ); }
    }
  }
}

} // namespace Game::Sys
