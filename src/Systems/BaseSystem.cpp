#include <Audio/SoundBank.hpp>

#include <Components/Altar/MultiBlock.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/Crypt/BuildingMultiBlock.hpp>
#include <Components/Crypt/BuildingSegment.hpp>
#include <Components/Player/RepelNpc.hpp>
#include <Components/Player/ToxicImmunity.hpp>
#include <Components/Position.hpp>
#include <Components/Ruin/BuildingMultiBlock.hpp>
#include <Components/Ruin/BuildingSegment.hpp>
#include <Components/Spring/HealingSpringBuildingMultiBlock.hpp>
#include <Components/Spring/HealingSpringBuildingSegment.hpp>
#include <Components/Stats/ActionReward.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/SideEffects.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
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

void BaseSystem::play_action_sfx( const Cmp::ActionSfx &sfx )
{
  if ( not sfx.final.empty() ) { m_sound_bank.get_effect( sfx.final ).play(); }
}

void BaseSystem::play_action_oneshot_sfx( const Cmp::ActionSfx &sfx )
{
  if ( not sfx.oneshot.empty() ) { m_sound_bank.get_effect( sfx.oneshot ).play(); }
}

void BaseSystem::play_action_loop_sfx( const Cmp::ActionSfx &sfx )
{
  if ( sfx.loop.empty() ) return;
  auto &sound = m_sound_bank.get_effect( sfx.loop );
  if ( sound.getStatus() != sf::Sound::Status::Playing ) { sound.play(); }
}

void BaseSystem::stop_action_loop_sfx( const Cmp::ActionSfx &sfx )
{
  if ( not sfx.loop.empty() ) { m_sound_bank.get_effect( sfx.loop ).stop(); }
}

void BaseSystem::play_action_sfx( const Cmp::BaseAction &action ) { play_action_sfx( action.sfx() ); }
void BaseSystem::play_action_loop_sfx( const Cmp::BaseAction &action ) { play_action_loop_sfx( action.sfx() ); }
void BaseSystem::stop_action_loop_sfx( const Cmp::BaseAction &action ) { stop_action_loop_sfx( action.sfx() ); }

void BaseSystem::trigger_side_effects( const Cmp::BaseAction &action, const Cmp::Position &pos_cmp, entt::entity source_entt )
{
  const auto &effects = action.side_effects();
  const auto &player_stats = Utils::Player::get_stats( reg() );
  const int player_luck = player_stats.luck();
  // the "if" condition gates the spawn before its probability is rolled; max_instances is enforced by the CreateNpcEvent handler
  if ( effects.spawn_npc and not effects.spawn_npc->condition_met( player_stats ) )
  {
    SPDLOG_DEBUG( "Spawn condition not met for {}.", effects.spawn_npc->npc_type );
  }
  else if ( effects.spawn_npc and not effects.spawn_npc->roll( player_luck ) )
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
    get_systems_event_queue().trigger(
        Events::CreateNpcEvent( Sys::NpcKey( effects.spawn_npc->npc_type ), pos_cmp, effects.spawn_npc->max_instances, source_uuid, {}, placement ) );
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
  // applying again restarts the timer. PlayerSystem keeps the PlayerStats flag in sync, but set it now so immunity
  // applies from this frame
  if ( effects.toxic_immunity and effects.toxic_immunity->roll( player_luck ) )
  {
    reg().emplace_or_replace<Cmp::Player::ToxicImmunity>( Utils::Player::get_entity( reg() ),
                                                          sf::seconds( static_cast<float>( effects.toxic_immunity->tick ) ) );
    Utils::Player::get_stats( reg() ).set_toxic_immune( true );
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
  // an inventory reward the player has no free hand for is dropped once nearby instead. Every drop here is a reward for
  // the player, so it may be picked up straight into an empty slot (items.json "auto_pickup")
  const int quantity = reward.target == Cmp::ActionReward::Target::NEARBY ? reward.quantity : 1;
  for ( int i = 0; i < quantity; ++i )
  {
    get_systems_event_queue().trigger( Events::CreateItemEvent( source, item_store.pick_item( reward.item ), "drop_loot", 0.f, /*spawn_nearby=*/true,
                                                                reward.max_instances, source_entt, on_created, /*sting=*/true,
                                                                /*player_reward=*/true ) );
  }
}

template <class MULTIBLOCK, class MBSEGMENT>
void BaseSystem::update_mb_zorder()
{
  auto player_pos = Utils::Player::get_position( reg() );

  for ( auto [mb_entt, mb_cmp, mb_z_cmp] : reg().view<MULTIBLOCK, Cmp::ZOrderValue>().each() )
  {
    if ( not player_pos.findIntersection( mb_cmp ) ) continue;
    auto segment_view = reg().view<MBSEGMENT, Cmp::Position, Cmp::ZOrderValue>();
    for ( auto [segment_entt, segment_cmp, segment_pos_cmp, segment_z_cmp] : segment_view.each() )
    {
      if ( not player_pos.findIntersection( segment_pos_cmp ) ) continue;
      mb_z_cmp.set( segment_z_cmp.get() );
      SPDLOG_DEBUG( "Updated zorder to {}", segment_z_cmp.get() );
    }
  }
}

template void BaseSystem::update_mb_zorder<Cmp::Crypt::BuildingMultiBlock, Cmp::Crypt::BuildingSegment>();
template void BaseSystem::update_mb_zorder<Cmp::Ruin::BuildingMultiBlock, Cmp::Ruin::BuildingSegment>();
template void BaseSystem::update_mb_zorder<Cmp::Altar::MultiBlock, Cmp::Altar::Segment>();
template void BaseSystem::update_mb_zorder<Cmp::HealingSpringBuildingMultiBlock, Cmp::HealingSpringBuildingSegment>();

// initialised by first call to getEventDispatcher()
std::unique_ptr<entt::dispatcher> BaseSystem::m_systems_event_queue = nullptr;

} // namespace Game::Sys