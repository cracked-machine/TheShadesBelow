
#include <Components/Player/RepelNpc.hpp>
#include <Components/Position.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/UUID.hpp>
#include <Events/AddInventoryEvent.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/CreateNpcEvent.hpp>
#include <Events/LightningEvent.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Utils/Player.hpp>

#include <entt/entity/registry.hpp>

// clang-format off
namespace Audio  { class SoundBank; }
// clang-format on

namespace Game::Sys
{

BaseSystem::BaseSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : m_reg( reg ),
      m_window( window ),
      m_sound_bank( sound_bank )
{
  SPDLOG_DEBUG( "BaseSystem constructor called" );
}

void BaseSystem::trigger_side_effects( const Cmp::BaseAction &action, const Cmp::Position &pos_cmp, entt::entity source_entt )
{
  const auto &effects = action.side_effects();
  const int player_luck = Utils::Player::get_stats( reg() ).luck();
  if ( effects.spawn_npc and not effects.spawn_npc->roll( player_luck ) )
  {
    SPDLOG_WARN( "Spawn roll failed for {} (chance {}).", effects.spawn_npc->npc_type, effects.spawn_npc->chance( player_luck ) );
  }
  else if ( effects.spawn_npc )
  {
    Cmp::UUID source_uuid;
    if ( reg().valid( source_entt ) )
    {
      if ( const auto *uuid_cmp = reg().try_get<Cmp::UUID>( source_entt ) ) { source_uuid = *uuid_cmp; }
    }
    // beside the source (plant, player...) by default, or centred on it (e.g. on an altar)
    const auto placement = effects.spawn_npc->location == Cmp::SpawnNpcEffect::Location::CENTER ? Events::CreateNpcEvent::Placement::CENTER
                                                                                                 : Events::CreateNpcEvent::Placement::NEARBY;
    get_systems_event_queue().trigger( Events::CreateNpcEvent( Sys::NpcKey( effects.spawn_npc->npc_type ), pos_cmp, effects.spawn_npc->max_instances,
                                                               source_uuid, {}, placement ) );
  }
  // a null target lets LightningSystem pick a working lightning rod in view, otherwise the player
  if ( effects.lightning and effects.lightning->roll( player_luck ) ) { get_systems_event_queue().trigger( Events::LightningEvent{} ); }
  // only npc repel is implemented so far; drinking again while repelled restarts the timer
  if ( effects.movement and effects.movement->roll( player_luck ) )
  {
    const auto &npc_effect = effects.movement->npc;
    if ( npc_effect and npc_effect->type == Cmp::MovementEffect::Npc::Type::REPEL )
    {
      reg().emplace_or_replace<Cmp::Player::RepelNpc>( Utils::Player::get_entity( reg() ), sf::seconds( static_cast<float>( npc_effect->tick ) ) );
    }
  }
}

void BaseSystem::grant_item_reward( const Cmp::ActionReward &reward, const Cmp::Position &source, entt::entity source_entt,
                                    const std::function<void( entt::entity )> &on_created )
{
  const auto &item_store = Sys::ItemStore::instance();
  if ( reward.target == Cmp::ActionReward::Target::INVENTORY )
  {
    if ( const auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() ); inventory_type == "" )
    {
      get_systems_event_queue().trigger( Events::AddInventoryEvent( item_store.pick_item( reward.item ) ) );
      return;
    }
  }
  // an inventory reward the player has no free hand for is dropped once nearby instead
  const int quantity = reward.target == Cmp::ActionReward::Target::NEARBY ? reward.quantity : 1;
  for ( int i = 0; i < quantity; ++i )
  {
    get_systems_event_queue().trigger(
        Events::CreateItemEvent( source, item_store.pick_item( reward.item ), "drop_loot", 0.f, /*spawn_nearby=*/true, reward.max_instances, source_entt,
                                                               on_created ) );
  }
}

// initialised by first call to getEventDispatcher()
std::unique_ptr<entt::dispatcher> BaseSystem::m_systems_event_queue = nullptr;

} // namespace Game::Sys