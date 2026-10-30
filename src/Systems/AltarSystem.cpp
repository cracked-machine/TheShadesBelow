#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteAlpha.hpp>
#include <Components/Altar/AltarActivationTimer.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/Altar/Sacrifice.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/DestroyedObstacle.hpp>
#include <Components/Inventory/FlashUIExtraLife.hpp>
#include <Components/Inventory/FlashUIHealth.hpp>
#include <Components/Inventory/FlashUIInventory.hpp>
#include <Components/Inventory/FlashUIWealth.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/LootContainer.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Particle/FlameParticleSprite.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/ExtraLife.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/ToxicImmunity.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Stats/PlayerStats.hpp>
#include <Components/Stats/SacrificeAction.hpp>
#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Venom.hpp>
#include <Components/Toxicity/Vertigo.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/DestroyNpcEvent.hpp>
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
    check_player_altar_sacrifice( altar_entity, altar_cmp );
  }
}

void AltarSystem::update( sf::Time dt )
{
  if ( auto *altar_timer = reg().try_get<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) ) ) *altar_timer += dt;
  // tidy up any dead altar sacrifice animations
  auto altar_sacrifice_view = reg().view<Cmp::Altar::Sacrifice, Cmp::AnimData>();
  for ( auto [altar_sacrifice_entt, altar_sacrifice_cmp, altar_sacrifice_anim_cmp] : altar_sacrifice_view.each() )
  {
    if ( not altar_sacrifice_anim_cmp.m_enabled )
    {
      if ( reg().valid( altar_sacrifice_entt ) ) { reg().destroy( altar_sacrifice_entt ); }
    }
  }
}

void AltarSystem::check_player_altar_sacrifice( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp )
{
  auto *altar_timer = reg().try_get<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) );
  if ( altar_timer and *altar_timer < altar_timer->get_timeout() ) return;

  auto *altar_uuid_cmp = reg().try_get<Cmp::UUID>( altar_entity );
  if ( not altar_uuid_cmp ) throw std::runtime_error( "Altar does not have UUID" );

  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type == "" ) return;
  const auto &item = Sys::ItemStore::instance().get( inventory_type );
  if ( not item.is_sacrificial() ) return;

  auto is_carrying_tool = [&]()
  {
    return item.item_type.contains( "item.axe" ) or item.item_type.contains( "item.pickaxe" ) or item.item_type.contains( "item.shovel" ) or
           item.item_type.contains( "item.dowsingrod" );
  };

  // only spent tools can be sacrificed
  if ( is_carrying_tool() and Utils::Player::get_inventory_wear_level( reg() ) > 0 ) { return; }

  // the altar refuses an offering whose NPC it has already summoned up to max_instances, otherwise the item is consumed for nothing
  if ( const auto spawn_npc = item.get_sacrifice_npc_spawn_effect();
       spawn_npc and spawn_npc->max_instances > 0 and Utils::Npc::count_spawned_by( reg(), *altar_uuid_cmp ) >= spawn_npc->max_instances )
  {
    return;
  }

  // an offering whose rewards are all for another kind of altar is refused (e.g. nightshade at the crypt altar)
  const auto reward = item.get_sacrifice_reward( altar_cmp.get_kind() );
  if ( not reward and not item.get_rewards<Cmp::SacrificeAction>().empty() ) return;

  // the altar may refuse the offering (e.g. a relic once the exitkey has dropped), so only consume the item once it has accepted
  if ( not grant_sacrifice_reward( altar_entity, altar_cmp, *altar_uuid_cmp, reward ) ) return;

  SPDLOG_INFO( "Player made an offering: {}", item.item_type );
  const auto sacrifice_action = Utils::Player::apply_action_from_item_store<Cmp::SacrificeAction>( reg(), item.item_type );
  trigger_side_effects( sacrifice_action, Cmp::Position( altar_cmp.position, altar_cmp.size ), altar_entity );
  Factory::Player::destroy_inventory( reg(), item.item_type );
}

bool AltarSystem::grant_sacrifice_reward( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid,
                                          const std::optional<Cmp::ActionReward> &reward )
{
  if ( not reward )
  {
    play_sacrifice_anim( altar_cmp, SacrificeAnimType::DEFAULT );
    return true;
  }

  // Each offering has exactly one reward (enforced by ItemStore), see "sacrifice_action.reward" in items.json
  if ( not reward->item.empty() )
  {
    sacrifice_for_item( altar_cmp, reward->item );
    return true;
  }
  if ( reward->wealth > 0 )
  {
    sacrifice_for_wealth( altar_cmp, reward->wealth );
    return true;
  }

  using AltarEffect = Cmp::ActionReward::AltarEffect;
  switch ( reward->altar )
  {
    case AltarEffect::CANDLE:
      return sacrifice_relic( altar_entity, altar_cmp, altar_uuid );
    case AltarEffect::BANISH:
      sacrifice_for_banish( altar_cmp );
      return true;
    case AltarEffect::EXTRA_LIFE:
      return sacrifice_for_extra_life( altar_entity, altar_cmp, altar_uuid );
    case AltarEffect::CRYPT_KEY:
      return sacrifice_exitkey( altar_entity, altar_cmp, altar_uuid );
    case AltarEffect::TOXIC_IMMUNITY:
      sacrifice_for_toxic_immunity( altar_cmp, reward->tick );
      return true;
    case AltarEffect::NONE:
      play_sacrifice_anim( altar_cmp, SacrificeAnimType::DEFAULT );
      return true;
  }
  return false;
}

void AltarSystem::sacrifice_for_item( const Cmp::Altar::MultiBlock &altar_cmp, const Sys::ItemKey &reward_item )
{
  play_sacrifice_anim( altar_cmp, SacrificeAnimType::DEFAULT );
  get_systems_event_queue().trigger( Events::CreateItemEvent( Utils::Player::get_position( reg() ), reward_item, "drop_loot" ) );
}

void AltarSystem::sacrifice_for_wealth( const Cmp::Altar::MultiBlock &altar_cmp, int wealth )
{
  play_sacrifice_anim( altar_cmp, SacrificeAnimType::DEFAULT );
  Utils::Player::get_wealth( reg() ).wealth += wealth;
  m_sound_bank.get_effect( "get_loot" ).play();
  // signal UI to flash
  reg().emplace_or_replace<Cmp::FlashUIWealth>( reg().create() );
}

void AltarSystem::sacrifice_for_toxic_immunity( const Cmp::Altar::MultiBlock &altar_cmp, int tick )
{
  play_sacrifice_anim( altar_cmp, SacrificeAnimType::DEFAULT );
  // sacrificing again restarts the timer. PlayerSystem keeps the PlayerStats flag in sync, but set it now so
  // immunity applies from this frame
  reg().emplace_or_replace<Cmp::Player::ToxicImmunity>( Utils::Player::get_entity( reg() ), sf::seconds( static_cast<float>( tick ) ) );
  Utils::Player::get_stats( reg() ).set_toxic_immune( true );
}

void AltarSystem::sacrifice_for_banish( const Cmp::Altar::MultiBlock &altar_cmp )
{
  // player kills all spawned ghosts in the game
  play_sacrifice_anim( altar_cmp, SacrificeAnimType::DEFAULT );
  for ( auto [npc_entt, npc_cmp, npc_pos_cmp, npc_anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::AnimData>().each() )
  {
    if ( not npc_anim_cmp.m_sprite_type.contains( "ghost" ) ) continue;
    get_systems_event_queue().trigger( Events::DestroyNpcEvent( npc_entt, Sys::SpriteKey( "sprite.death.anim.banish" ) ) );
  }
  m_sound_bank.get_effect( "witches_jar_sacrifice" ).play();
}

bool AltarSystem::sacrifice_for_extra_life( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid )
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
  m_sound_bank.get_effect( "witches_jar_sacrifice" ).play();
  return true;
}

bool AltarSystem::sacrifice_relic( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid )
{
  const uint8_t sacrifice_count = altar_cmp.get_sacrifice_count();
  if ( sacrifice_count >= altar_cmp.get_exitkey_drop_threshold() ) return false;

  auto *anim_cmp = reg().try_get<Cmp::AnimData>( altar_entity );
  if ( not anim_cmp ) return false;
  // the altar may be lit by an extra life, but its flame offsets are defined on the inactive sprite
  const Sys::SpriteKey inactive_sprite_type = Factory::Altar::get_altar_sprite( anim_cmp->m_sprite_type, false );
  if ( inactive_sprite_type == anim_cmp->m_sprite_type and not inactive_sprite_type.str().ends_with( ".inactive" ) )
  {
    SPDLOG_WARN( "Altar sprite {} has no active/inactive variant", inactive_sprite_type.str() );
    return false;
  }

  reg().patch<Cmp::Altar::MultiBlock>( altar_entity, []( Cmp::Altar::MultiBlock &cmp ) { cmp.increment_sacrifice_count(); } );
  const auto &flame_offsets = Sys::SpriteStore::instance().get( inactive_sprite_type ).particle_offsets();
  if ( sacrifice_count < flame_offsets.size() )
  {
    sf::Vector2f flame_ps_pos = altar_cmp.position + flame_offsets[sacrifice_count];
    Factory::Particle::add_flame( m_reg, Factory::Altar::kCandleFlameParticleTag, altar_uuid, flame_ps_pos, 5000, Cmp::Particle::kWorldScalePreset );
  }
  else { SPDLOG_WARN( "{} has no particle_offsets entry for sacrifice {}", inactive_sprite_type.str(), sacrifice_count ); }
  SPDLOG_DEBUG( "Altar activated to state {}.", altar_cmp.get_sacrifice_count() );
  play_sacrifice_anim( altar_cmp, SacrificeAnimType::DEFAULT );

  if ( altar_cmp.get_sacrifice_count() == altar_cmp.get_exitkey_drop_threshold() and not altar_cmp.is_exitkey_lockout() )
  {
    // all the candles are now lit in their default colour
    drop_reward_item( "item.exitkey" );
    altar_cmp.set_exitkey_lockout();
    m_sound_bank.get_effect( "crypt_altar_activate" ).play();
  }
  return true;
}

bool AltarSystem::sacrifice_exitkey( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid )
{
  // all the relics must be sacrificed first
  const uint8_t sacrifice_count = altar_cmp.get_sacrifice_count();
  if ( sacrifice_count < altar_cmp.get_exitkey_drop_threshold() or sacrifice_count >= altar_cmp.get_cryptkey_drop_threshold() ) return false;

  reg().patch<Cmp::Altar::MultiBlock>( altar_entity, []( Cmp::Altar::MultiBlock &cmp ) { cmp.increment_sacrifice_count(); } );

  // turn this altar's candle flames blue
  for ( auto [particle_entt, particle_cmp, particle_uuid_cmp] : reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
  {
    if ( particle_uuid_cmp != altar_uuid ) continue;
    if ( particle_cmp.sprite->get_tag() != Factory::Altar::kCandleFlameParticleTag ) continue;
    auto *flame_particle = dynamic_cast<Cmp::Particle::FlameParticleSprite *>( particle_cmp.sprite.get() );
    if ( flame_particle ) { flame_particle->m_final_flame_color = { 0, 0, 255 }; }
  }
  SPDLOG_DEBUG( "Altar activated to state {}.", altar_cmp.get_sacrifice_count() );
  play_sacrifice_anim( altar_cmp, SacrificeAnimType::KEY );

  if ( altar_cmp.get_sacrifice_count() >= altar_cmp.get_cryptkey_drop_threshold() and not altar_cmp.is_cryptkey_lockout() )
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
  Factory::Particle::add_crypt_altar_ps( m_reg, Factory::Altar::kExtraLifeParticleTag, 0.5, 25.f, altar_uuid, altar_cmp.position + particle_offset, 5000 );
  m_sound_bank.get_effect( "crypt_altar_activate" ).play();
}

void AltarSystem::play_sacrifice_anim( const Cmp::Altar::MultiBlock &altar_cmp, SacrificeAnimType sacrifice_anim_type )
{
  Sys::SpriteKey sprite_type = ( sacrifice_anim_type == SacrificeAnimType::KEY ) ? "sprite.graveyard.altar.key.anim"
                                                                                 : "sprite.graveyard.altar.relic.anim";

  float altar_sacrifice_anim_height = Sys::SpriteStore::instance().get( sprite_type ).sprite_size().y;
  // get the center (topleft coord), then adjust to center the altar_sacrifice_anim, then adjust for altar_sacrifice_anim height
  Cmp::Position new_pos( altar_cmp.getCenter() - sf::Vector2{ 7.f, 28.f } - sf::Vector2{ 0.f, altar_sacrifice_anim_height },
                         Constants::kGridSizePxF );
  m_sound_bank.get_effect( "shrine_lighting" ).play();
  Factory::Altar::create_altar_sacrifice_anim( reg(), new_pos, sprite_type );

  reg().emplace_or_replace<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) );
}

void AltarSystem::drop_reward_item( const Sys::ItemKey &item_key )
{
  get_systems_event_queue().trigger( Events::CreateItemEvent( Utils::Player::get_position( reg() ), item_key, "drop_loot" ) );
  // signal UI to flash
  reg().emplace_or_replace<Cmp::FlashUIInventory>( reg().create() );
}

} // namespace Game::Sys
