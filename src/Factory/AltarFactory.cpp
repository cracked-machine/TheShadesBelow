#include <Components/Altar/MultiBlock.hpp>
#include <Components/Altar/Offering.hpp>
#include <Components/AnimData.hpp>
#include <Components/Particle/SpriteOwner.hpp>
#include <Components/ZOrderValue.hpp>
#include <Factory/AltarFactory.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <entt/entity/registry.hpp>

#include <stdexcept>
#include <string>
#include <string_view>

namespace Game::Factory::Altar
{

void create_altar_offering_anim( entt::registry &reg, Cmp::Position pos, Sys::SpriteKey anim_type )
{
  entt::entity entt = reg.create();
  reg.emplace_or_replace<Cmp::Altar::Offering>( entt );
  reg.emplace_or_replace<Cmp::Position>( entt, pos.position, pos.size );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( entt, Cmp::AnimData::Config{
    .sprite_type        = anim_type,
    .anim_type = Cmp::AnimType::ONESHOTRESET
  });
  // clang-format off

  reg.emplace_or_replace<Cmp::ZOrderValue>( entt, pos.position.y * 4.f );
}

Sys::SpriteKey get_altar_sprite( const Sys::SpriteKey &sprite_type, bool active )
{
  std::string sprite_str = sprite_type.str();
  if ( sprite_str.ends_with( ".inactive" ) ) { sprite_str.erase( sprite_str.size() - std::string_view( ".inactive" ).size() ); }
  else if ( sprite_str.ends_with( ".active" ) ) { sprite_str.erase( sprite_str.size() - std::string_view( ".active" ).size() ); }
  else { return sprite_type; }
  return Sys::SpriteKey( sprite_str + ( active ? ".active" : ".inactive" ) );
}

Sys::SpriteKey get_altar_talisman_sprite( const Sys::ItemKey &item_key )
{
  constexpr std::string_view kItemPrefix = "sprite.item.";
  constexpr std::string_view kAltarPrefix = "sprite.altar.";

  const std::string &item_sprite = Sys::ItemStore::instance().get( item_key ).sprite_type.str();
  if ( not item_sprite.starts_with( kItemPrefix ) )
  {
    throw std::runtime_error( "Talisman '" + item_key.str() + "' has sprite '" + item_sprite + "', which does not start with '" +
                              std::string( kItemPrefix ) + "'" );
  }

  const Sys::SpriteKey altar_sprite( std::string( kAltarPrefix ) + item_sprite.substr( kItemPrefix.size() ) );
  // throws if there is no altar variant of this talisman's sprite
  [[maybe_unused]] const auto &sheet = Sys::SpriteStore::instance().get( altar_sprite );
  return altar_sprite;
}

void deactivate_altars( entt::registry &reg )
{
  for ( auto [ps_entt, ps_cmp] : reg.view<Cmp::Particle::SpriteOwner>().each() )
  {
    if ( ps_cmp.sprite->get_tag() == kExtraLifeParticleTag ) reg.destroy( ps_entt );
  }

  for ( auto [altar_mb_entt, altar_mb_cmp, altar_anim_cmp] : reg.view<Cmp::Altar::MultiBlock, Cmp::AnimData>().each() )
  {
    altar_anim_cmp.m_sprite_type = get_altar_sprite( altar_anim_cmp.m_sprite_type, false );
  }
}

} // namespace Game::Factory::Altar
