#include <Components/Inventory/WorldItem.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ForageAction.hpp>
#include <Components/Stats/OfferingAction.hpp>
#include <Components/Stats/SpawnAction.hpp>

#include <algorithm>

namespace Game::Cmp
{

bool WorldItem::has_wear() const
{
  return std::ranges::any_of( m_actions.map(), []( const auto &pair ) { return pair.second.action.wear().has_value(); } );
}

bool WorldItem::wears_in_world() const { return get_wear<SpawnAction>().has_value(); }

sf::Time WorldItem::world_expiry() const { return sf::seconds( get_wear<SpawnAction>().value_or( ActionWear{} ).expiry_seconds ); }

bool WorldItem::wears_in_inventory() const { return get_wear<CarryAction>().has_value(); }

sf::Time WorldItem::inventory_expiry() const { return sf::seconds( get_wear<CarryAction>().value_or( ActionWear{} ).expiry_seconds ); }

bool WorldItem::wears_only_when_active() const { return get_wear<CarryAction>().value_or( ActionWear{} ).active_only; }

std::optional<ActionReward> WorldItem::get_offering_reward( Altar::Kind location ) const
{
  if ( not has_action<OfferingAction>() ) return std::nullopt;
  return at<OfferingAction>().action.reward( location );
}

std::optional<SpawnNpcEffect> WorldItem::get_offering_npc_spawn_effect() const { return get_spawn_npc_effect<OfferingAction>(); }

bool WorldItem::is_offerable() const { return has_action<OfferingAction>() and not at<OfferingAction>().action.is_zero(); }

Sys::ItemKey WorldItem::get_forage_output() const
{
  const auto reward = get_reward<ForageAction>();
  return reward ? reward->item : Sys::ItemKey();
}

Sys::ItemKey WorldItem::get_axe_output() const
{
  const auto reward = get_reward<DestroyAction>();
  return reward ? reward->item : Sys::ItemKey();
}

bool WorldItem::stings() const
{
  return ( has_action<SpawnAction>() and at<SpawnAction>().action.health() < 0 ) or
         ( has_action<CarryAction>() and at<CarryAction>().action.health() < 0 );
}

} // namespace Game::Cmp
