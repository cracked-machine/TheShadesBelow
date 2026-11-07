#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteAlpha.hpp>
#include <Components/Altar/Activated.hpp>
#include <Components/Altar/AltarActivationTimer.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/Altar/Offering.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/Altar/TalismanCount.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/DestroyedObstacle.hpp>
#include <Components/Inventory/FlashUIAnima.hpp>
#include <Components/Inventory/FlashUIExtraLife.hpp>
#include <Components/Inventory/FlashUIHealth.hpp>
#include <Components/Inventory/FlashUIInventory.hpp>
#include <Components/Inventory/FlashUIWealth.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/LootContainer.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/SummonOffering.hpp>
#include <Components/Particle/FlameParticleSprite.hpp>
#include <Components/Persistent/AltarOfferingCooldown.hpp>
#include <Components/Persistent/AltarSummonOfferingCooldown.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/ExtraLife.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/ToxicImmunity.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Stats/PlayerStats.hpp>
#include <Components/Stats/OfferingAction.hpp>
#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Venom.hpp>
#include <Components/Toxicity/Vertigo.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/AltarActivatedEvent.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/DestroyNpcEvent.hpp>
#include <Events/InsufficientAnimaEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Factory/AltarFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <SFML/System/Time.hpp>
#include <Systems/AltarSystem.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Npc.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <stdexcept>

namespace Game::Sys
{

AltarSystem::AltarSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&AltarSystem::on_player_action>( this );
}

void AltarSystem::on_player_action( Events::PlayerActionEvent ev )
{
  if ( ev.action != Events::PlayerActionEvent::GameActions::ACTIVATE ) return;

  for ( auto [altar_entity, altar_cmp] : reg().view<Cmp::Altar::MultiBlock>().each() )
  {
    if ( not Utils::Player::is_player_near( reg(), altar_cmp ) ) continue;

    SPDLOG_DEBUG( "Player collided with Altar at ({}, {})", altar_cmp.position.x, altar_cmp.position.y );
    check_player_altar_offering( altar_entity, altar_cmp );
  }
}

void AltarSystem::update( sf::Time dt )
{
  if ( auto *altar_timer = reg().try_get<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) ) ) *altar_timer += dt;
  // tidy up any dead altar offering animations
  auto altar_offering_view = reg().view<Cmp::Altar::Offering, Cmp::AnimData>();
  for ( auto [altar_offering_entt, altar_offering_cmp, altar_offering_anim_cmp] : altar_offering_view.each() )
  {
    if ( not altar_offering_anim_cmp.m_enabled )
    {
      if ( reg().valid( altar_offering_entt ) ) { reg().destroy( altar_offering_entt ); }
    }
  }
}

void AltarSystem::check_player_altar_offering( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp )
{
  auto *altar_timer = reg().try_get<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) );
  if ( altar_timer )
  {
    // summoning offerings (e.g. Azazel's jewelry) come in batches, so they can be offered in quicker succession
    const auto inventory_entt = Utils::Player::get_inventory_entt( reg() );
    const bool is_summon_offering = reg().valid( inventory_entt ) and reg().all_of<Cmp::Npc::SummonOffering>( inventory_entt );
    const sf::Time timeout = is_summon_offering
                                 ? sf::seconds( Sys::PersistSystem::get<Cmp::Persist::AltarSummonOfferingCooldown>( reg() ).get_value() )
                                 : sf::seconds( Sys::PersistSystem::get<Cmp::Persist::AltarOfferingCooldown>( reg() ).get_value() );
    if ( *altar_timer < timeout ) return;
  }

  auto *altar_uuid_cmp = reg().try_get<Cmp::UUID>( altar_entity );
  if ( not altar_uuid_cmp ) throw std::runtime_error( "Altar does not have UUID" );

  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type == "" ) return;
  const auto &item = Sys::ItemStore::instance().get( inventory_type );
  if ( not item.is_offerable() ) return;

  auto is_carrying_tool = [&]()
  {
    return item.item_type.contains( "item.axe" ) or item.item_type.contains( "item.pickaxe" ) or item.item_type.contains( "item.shovel" ) or
           item.item_type.contains( "item.dowsingrod" );
  };

  // only spent tools can be offered
  if ( is_carrying_tool() and Utils::Player::get_inventory_wear_level( reg() ) > 0 ) { return; }

  // the altar refuses an offering whose NPC it has already summoned up to max_instances, otherwise the item is consumed for nothing
  if ( const auto spawn_npc = item.get_offering_npc_spawn_effect();
       spawn_npc and spawn_npc->max_instances > 0 and Utils::Npc::count_spawned_by( reg(), *altar_uuid_cmp ) >= spawn_npc->max_instances )
  {
    return;
  }

  // an offering whose rewards are all for another kind of altar is refused (e.g. nightshade at the crypt altar)
  const auto reward = item.get_offering_reward( altar_cmp.get_kind() );
  if ( not reward and not item.get_rewards<Cmp::OfferingAction>().empty() ) return;

  // the altar may refuse the offering (e.g. a relic once the exitkey has dropped), so only consume the item once it has accepted
  if ( not grant_offering_reward( altar_entity, altar_cmp, *altar_uuid_cmp, reward ) ) return;

  SPDLOG_INFO( "Player made an offering: {}", item.item_type );
  const auto offering_action = Utils::Player::apply_action_from_item_store<Cmp::OfferingAction>( reg(), item.item_type );
  trigger_side_effects( offering_action, Cmp::Position( altar_cmp.position, altar_cmp.size ), altar_entity );
  Factory::Player::destroy_inventory( reg(), item.item_type );
  // the offering has left the inventory, so an inventory reward item can take its place
  if ( reward and not reward->item.empty() )
  {
    // items dropped by a summoning offering are bound to the summoned NPC, e.g. Azazel stays until its jewelry is offered
    std::function<void( entt::entity )> on_created;
    if ( offering_action.side_effects().spawn_npc )
    {
      on_created = [this, summoner = *altar_uuid_cmp]( entt::entity item_entt )
      { reg().emplace_or_replace<Cmp::Npc::SummonOffering>( item_entt, summoner ); };
    }
    offering_for_item( altar_cmp, *reward, on_created );
  }
}

bool AltarSystem::grant_offering_reward( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid,
                                          const std::optional<Cmp::ActionReward> &reward )
{
  if ( not reward )
  {
    play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
    return true;
  }
  if ( not reward->item.empty() )
  {
    play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
    return true;
  }
  if ( reward->wealth > 0 )
  {
    offering_for_wealth( altar_cmp, reward->wealth );
    return true;
  }

  using AltarEffect = Cmp::ActionReward::AltarEffect;
  switch ( reward->altar )
  {
    case AltarEffect::TALISMAN:
      return offering_talisman( altar_entity, altar_cmp, altar_uuid );
      return false;
    case AltarEffect::BANISH:
      offering_for_banish( altar_cmp );
      return true;
    case AltarEffect::EXTRA_LIFE:
      return offering_for_extra_life( altar_entity, altar_cmp, altar_uuid );
    case AltarEffect::CRYPT_KEY:
      // return offering_exitkey( altar_entity, altar_cmp, altar_uuid );
      return false;
    case AltarEffect::TOXIC_IMMUNITY:
      offering_for_toxic_immunity( altar_cmp, reward->tick );
      return true;
    case AltarEffect::NONE:
      play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
      return true;
  }
  return false;
}

void AltarSystem::offering_for_item( const Cmp::Altar::MultiBlock &altar_cmp, const Cmp::ActionReward &reward,
                                      const std::function<void( entt::entity )> &on_created )
{
  if ( reward.target == Cmp::ActionReward::Target::DEFAULT )
  {
    get_systems_event_queue().trigger( Events::CreateItemEvent( Utils::Player::get_position( reg() ),
                                                                Sys::ItemStore::instance().pick_item( reward.item ), "drop_loot", 0.f, false, 0,
                                                                entt::null, on_created ) );
    return;
  }
  grant_item_reward( reward, Cmp::Position( altar_cmp.position, altar_cmp.size ), entt::null, on_created );
}

void AltarSystem::offering_for_wealth( const Cmp::Altar::MultiBlock &altar_cmp, int wealth )
{
  play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
  Utils::Player::get_wealth( reg() ).wealth += wealth;
  m_sound_bank.get_effect( "get_loot" ).play();
  // signal UI to flash
  reg().emplace_or_replace<Cmp::FlashUIWealth>( reg().create() );
}

void AltarSystem::offering_for_toxic_immunity( const Cmp::Altar::MultiBlock &altar_cmp, int tick )
{
  play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
  // offering again restarts the timer. PlayerSystem keeps the PlayerStats flag in sync, but set it now so
  // immunity applies from this frame
  reg().emplace_or_replace<Cmp::Player::ToxicImmunity>( Utils::Player::get_entity( reg() ), sf::seconds( static_cast<float>( tick ) ) );
  Utils::Player::get_stats( reg() ).set_toxic_immune( true );
}

void AltarSystem::offering_for_banish( const Cmp::Altar::MultiBlock &altar_cmp )
{
  // player kills all spawned ghosts in the game
  play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
  for ( auto [npc_entt, npc_cmp, npc_pos_cmp, npc_anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::AnimData>().each() )
  {
    if ( not npc_anim_cmp.m_sprite_type.contains( "ghost" ) ) continue;
    get_systems_event_queue().trigger( Events::DestroyNpcEvent( npc_entt, Sys::SpriteKey( "sprite.death.anim.banish" ) ) );
  }
  m_sound_bank.get_effect( "witches_jar_offering" ).play();
}

bool AltarSystem::offering_for_extra_life( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid )
{
  // refuse the offering while an extra life is already active, so it isn't wasted
  if ( Utils::Player::player_has_extra_life( reg() ) ) return false;

  Cmp::Player::ExtraLife extra_life;
  extra_life.altar_uuid = altar_uuid;
  if ( altar_cmp.get_kind() == Cmp::Altar::Kind::CRYPT )
  {
    if ( not Utils::Player::get_projected_position( reg() ).findIntersection( altar_cmp ) ) return false;
  }
  // the player respawns back here at a graveyard altar, or at the crypt start position
  else { extra_life.respawn_position = Utils::Player::get_position( reg() ).position; }

  // the altar stays lit while the extra life is held, see Factory::Player::remove_player_extra_life()
  activate_altar( altar_entity, altar_cmp, altar_uuid );
  // both go on the player so they transfer between scenes
  reg().emplace_or_replace<Cmp::FlashUIExtraLife>( Utils::Player::get_entity( reg() ) );
  reg().emplace_or_replace<Cmp::Player::ExtraLife>( Utils::Player::get_entity( reg() ), extra_life );
  m_sound_bank.get_effect( "witches_jar_offering" ).play();
  return true;
}

bool AltarSystem::offering_talisman( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid )
{

  bool altar_activated = reg().view<Cmp::Altar::Activated>().size() != 0;
  if ( altar_activated )
  {
    SPDLOG_WARN( "Altar already activated. Skipping" );
    return false;
  }
  auto *talisman_count_cmp = reg().try_get<Cmp::Altar::TalismanCount>( altar_entity );
  if ( not talisman_count_cmp ) { throw std::runtime_error( "Altar does not have an Cmp::Altar::TalismanCount component" ); }

  auto *anim_cmp = reg().try_get<Cmp::AnimData>( altar_entity );
  if ( not anim_cmp ) return false;
  // the altar may be lit by an extra life, but its flame offsets are defined on the inactive sprite
  const Sys::SpriteKey inactive_sprite_type = Factory::Altar::get_altar_sprite( anim_cmp->m_sprite_type, false );
  if ( inactive_sprite_type == anim_cmp->m_sprite_type and not inactive_sprite_type.str().ends_with( ".inactive" ) )
  {
    SPDLOG_WARN( "Altar sprite {} has no active/inactive variant", inactive_sprite_type.str() );
    return false;
  }

  auto [_, item_type, _] = Utils::Player::get_inventory( reg() );
  if ( item_type.empty() ) return false;

  const Sys::ItemKey talisman_key( item_type );
  // the talisman's index picks its candle, so the flame lights beside that talisman whatever order they are offered in
  const auto flame_idx = talisman_count_cmp->index( talisman_key );
  if ( not flame_idx )
  {
    SPDLOG_WARN( "{} is not a talisman for this altar", item_type );
    return false;
  }
  if ( talisman_count_cmp->activated( talisman_key ) ) return false;

  // the talisman's (negative) offering anima is its cost, paid by check_player_altar_offering() once the altar accepts it
  const int anima_cost = -Utils::Player::get_action_from_item_store<Cmp::OfferingAction>( talisman_key ).anima();
  if ( Utils::Player::get_stats( reg() ).anima() < anima_cost )
  {
    SPDLOG_INFO( "Not enough anima to activate {} (need {})", item_type, anima_cost );
    // signal UI to flash the anima meter and explain why
    reg().emplace_or_replace<Cmp::FlashUIAnima>( reg().create() );
    get_systems_event_queue().trigger( Events::InsufficientAnimaEvent( talisman_key ) );
    return false;
  }

  talisman_count_cmp->activate( talisman_key );
  SPDLOG_INFO( "Activated {} ({} of {})", item_type, talisman_count_cmp->count(), talisman_count_cmp->size() );

  const auto &flame_offsets = Sys::SpriteStore::instance().get( inactive_sprite_type ).particle_offsets();
  if ( *flame_idx < flame_offsets.size() )
  {
    sf::Vector2f flame_ps_pos = altar_cmp.position + flame_offsets[*flame_idx];
    Factory::Particle::add_flame( m_reg, Factory::Altar::kCandleFlameParticleTag, altar_uuid, flame_ps_pos, 5000, Cmp::Particle::kWorldScalePreset );
  }
  else { SPDLOG_WARN( "{} has no particle_offsets entry for talisman index {}", inactive_sprite_type.str(), *flame_idx ); }
  SPDLOG_DEBUG( "Altar activated to state {}.", altar_cmp.get_offering_count() );
  play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );

  if ( talisman_count_cmp->count() == talisman_count_cmp->size() )
  {
    reg().emplace_or_replace<Cmp::Altar::Activated>( altar_entity );
    m_sound_bank.get_effect( "crypt_altar_activate" ).play();
    // GraveSystem picks the trail start graves that TrailGuideSystem then guides the player to
    get_systems_event_queue().trigger( Events::AltarActivatedEvent{ Cmp::Position( altar_cmp.position, altar_cmp.size ) } );
  }
  return true;
}

bool AltarSystem::offering_exitkey( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid )
{
  // all the relics must be offered first
  const uint8_t offering_count = altar_cmp.get_offering_count();
  if ( offering_count < altar_cmp.get_exitkey_drop_threshold() or offering_count >= altar_cmp.get_cryptkey_drop_threshold() ) return false;

  reg().patch<Cmp::Altar::MultiBlock>( altar_entity, []( Cmp::Altar::MultiBlock &cmp ) { cmp.increment_offering_count(); } );

  // turn this altar's candle flames blue
  for ( auto [particle_entt, particle_cmp, particle_uuid_cmp] : reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
  {
    if ( particle_uuid_cmp != altar_uuid ) continue;
    if ( particle_cmp.sprite->get_tag() != Factory::Altar::kCandleFlameParticleTag ) continue;
    auto *flame_particle = dynamic_cast<Cmp::Particle::FlameParticleSprite *>( particle_cmp.sprite.get() );
    if ( flame_particle ) { flame_particle->m_final_flame_color = { 0, 0, 255 }; }
  }
  SPDLOG_DEBUG( "Altar activated to state {}.", altar_cmp.get_offering_count() );
  play_offering_anim( altar_cmp, OfferingAnimType::KEY );

  if ( altar_cmp.get_offering_count() >= altar_cmp.get_cryptkey_drop_threshold() and not altar_cmp.is_cryptkey_lockout() )
  {
    drop_reward_item( "item.cryptkey" );
    altar_cmp.set_cryptkey_lockout();
    m_sound_bank.get_effect( "crypt_altar_activate" ).play();
  }
  return true;
}

void AltarSystem::activate_altar( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid )
{
  auto *anim_cmp = reg().try_get<Cmp::AnimData>( altar_entity );
  if ( not anim_cmp ) return;
  const Sys::SpriteKey active_sprite_type = Factory::Altar::get_altar_sprite( anim_cmp->m_sprite_type, true );
  if ( anim_cmp->m_sprite_type == active_sprite_type ) return;

  anim_cmp->m_sprite_type = active_sprite_type;
  const sf::Vector2f particle_offset = ( altar_cmp.get_kind() == Cmp::Altar::Kind::CRYPT ) ? sf::Vector2f{ 8.f, 24.f } : sf::Vector2f{ 40.f, 21.f };
  Factory::Particle::add_crypt_altar_ps( m_reg, Factory::Altar::kExtraLifeParticleTag, 0.5, 25.f, altar_uuid, altar_cmp.position + particle_offset,
                                         5000 );
  m_sound_bank.get_effect( "crypt_altar_activate" ).play();
}

void AltarSystem::play_offering_anim( const Cmp::Altar::MultiBlock &altar_cmp, OfferingAnimType offering_anim_type )
{
  Sys::SpriteKey sprite_type = ( offering_anim_type == OfferingAnimType::KEY ) ? "sprite.graveyard.altar.key.anim"
                                                                                 : "sprite.graveyard.altar.relic.anim";

  float altar_offering_anim_height = Sys::SpriteStore::instance().get( sprite_type ).sprite_size().y;
  // get the center (topleft coord), then adjust to center the altar_offering_anim, then adjust for altar_offering_anim height
  Cmp::Position new_pos( altar_cmp.getCenter() - sf::Vector2{ 7.f, 28.f } - sf::Vector2{ 0.f, altar_offering_anim_height },
                         Constants::kGridSizePxF );
  m_sound_bank.get_effect( "shrine_lighting" ).play();
  Factory::Altar::create_altar_offering_anim( reg(), new_pos, sprite_type );

  reg().emplace_or_replace<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) );
}

void AltarSystem::drop_reward_item( const Sys::ItemKey &item_key )
{
  get_systems_event_queue().trigger( Events::CreateItemEvent( Utils::Player::get_position( reg() ), item_key, "drop_loot" ) );
  // signal UI to flash
  reg().emplace_or_replace<Cmp::FlashUIInventory>( reg().create() );
}

} // namespace Game::Sys
