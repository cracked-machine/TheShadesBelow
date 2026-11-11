#include <Components/Grave/CurseTablet.hpp>
#include <Factory/ItemFactory.hpp>

namespace Game::Factory::Item
{

void add_curse_tablet( entt::registry &reg, entt::entity item_entt, const Sys::ItemKey &item, size_t sprite_idx )
{
  using Type = Cmp::Grave::CurseTablet::Type;
  // each sprite variant needs one more hit than the last
  const int hits_required = static_cast<int>( sprite_idx ) + 1;
  if ( item.contains( "ghost" ) ) { reg.emplace_or_replace<Cmp::Grave::CurseTablet>( item_entt, hits_required, Type::GHOST ); }
  else if ( item.contains( "skeleton" ) ) { reg.emplace_or_replace<Cmp::Grave::CurseTablet>( item_entt, hits_required, Type::SKELETON ); }
  else if ( item.contains( "adder" ) ) { reg.emplace_or_replace<Cmp::Grave::CurseTablet>( item_entt, hits_required, Type::ADDER ); }
  else { SPDLOG_WARN( "No matching Curse Tablet key found for {}", item.str() ); }
}

} // namespace Game::Factory::Item