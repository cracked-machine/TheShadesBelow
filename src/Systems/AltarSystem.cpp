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
#include <Components/Player/Wealth.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
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
#include <Factory/LootFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
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
    check_player_altar_activation( altar_entity, altar_cmp );
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

void AltarSystem::check_player_altar_activation( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp )
{
  auto *altar_timer = reg().try_get<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) );
  if ( altar_timer and *altar_timer < altar_timer->get_timeout() ) return;

  auto *altar_uuid_cmp = reg().try_get<Cmp::UUID>( altar_entity );
  if ( not altar_uuid_cmp ) throw std::runtime_error( "Altar does not have UUID" );

  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type == "" ) return;
  const auto &item = Sys::ItemStore::instance().get( inventory_type );

  auto is_tool = [&]()
  {
    return item.item_type.contains( "item.axe" ) or item.item_type.contains( "item.pickaxe" ) or item.item_type.contains( "item.shovel" ) or
           item.item_type.contains( "item.dowsingrod" );
  };

  const auto reward = item.get_sacrifice_reward();
  if ( not reward ) return;

  // only spent tools can be sacrificed - check before the tool is consumed
  if ( is_tool() and Utils::Player::get_inventory_wear_level( reg() ) > 0 ) { return; }

  const auto sacrifice_action = Utils::Player::apply_action_from_item_store<Cmp::SacrificeAction>( reg(), item.item_type );
  trigger_side_effects( sacrifice_action, Utils::Player::get_position( reg() ) );
  SPDLOG_INFO( "Player made an offering: {} -> {}", item.item_type, reward->item );
  Factory::Player::destroy_inventory( reg(), item.item_type );

  // Each offering has exactly one reward (enforced by ItemStore), see "sacrifice_action.reward" in items.json
  using RewardSpecial = Cmp::ActionReward::Special;
  if ( not reward->item.empty() ) { sacrifice_for_item( altar_cmp, reward->item ); }
  else if ( reward->wealth > 0 ) { sacrifice_for_wealth( altar_cmp, reward->wealth ); }
  else
  {
    switch ( reward->special )
    {
      case RewardSpecial::ALTAR:
        sacrifice_relic( altar_entity, altar_cmp, *altar_uuid_cmp );
        break;
      case RewardSpecial::BANISH:
        sacrifice_witches_jar( altar_entity, altar_cmp, *altar_uuid_cmp, item );
        break;
      case RewardSpecial::CRYPT:
        sacrifice_exitkey( altar_entity, altar_cmp, *altar_uuid_cmp );
        break;
      case RewardSpecial::NONE:
        play_sacrifice_anim( altar_cmp, SacrificeAnimType::DEFAULT );
        break;
    }
  }
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

void AltarSystem::sacrifice_witches_jar( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid,
                                         const Cmp::WorldItem &item )
{
  auto *anim_cmp = reg().try_get<Cmp::AnimData>( altar_entity );
  if ( not anim_cmp ) return;

  if ( anim_cmp->m_sprite_type == "sprite.crypt.altar.inactive" )
  {
    if ( not Utils::Player::get_projected_position( reg() ).findIntersection( altar_cmp ) ) return;

    // player gains extra life protection
    anim_cmp->m_sprite_type = "sprite.crypt.altar.active";
    Factory::Particle::add_crypt_altar_ps( m_reg, "crypt.altar.particle.active", 0.5, 25.f, altar_uuid,
                                           { altar_cmp.position.x + 8.f, altar_cmp.position.y + 24.f }, 5000 );
    reg().emplace_or_replace<Cmp::FlashUIExtraLife>( reg().create() );
    reg().emplace_or_replace<Cmp::Player::ExtraLife>( Utils::Player::get_entity( reg() ) );
    m_sound_bank.get_effect( "crypt_altar_activate" ).play();
  }
  else if ( anim_cmp->m_sprite_type.contains( "sprite.graveyard.altar" ) )
  {
    // player kills all spawned ghosts in the game
    SPDLOG_INFO( "Player made an offering: {}", item.item_type );
    play_sacrifice_anim( altar_cmp, SacrificeAnimType::DEFAULT );
    for ( auto [npc_entt, npc_cmp, npc_pos_cmp, npc_anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::AnimData>().each() )
    {
      if ( not npc_anim_cmp.m_sprite_type.contains( "ghost" ) ) continue;
      get_systems_event_queue().trigger( Events::DestroyNpcEvent( npc_entt, Sys::SpriteKey( "sprite.death.anim.banish" ) ) );
    }
  }
  else { return; }

  m_sound_bank.get_effect( "witches_jar_sacrifice" ).play();
}

void AltarSystem::sacrifice_relic( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid )
{
  const uint8_t sacrifice_count = altar_cmp.get_sacrifice_count();
  if ( sacrifice_count >= altar_cmp.get_exitkey_drop_threshold() ) return;

  reg().patch<Cmp::Altar::MultiBlock>( altar_entity, []( Cmp::Altar::MultiBlock &cmp ) { cmp.increment_sacrifice_count(); } );
  sf::Vector2f flame_ps_pos = altar_cmp.position + altar_cmp.flame_offsets[sacrifice_count];
  Factory::Particle::add_flame( m_reg, "graveyard.altar.particle.flame", altar_uuid, flame_ps_pos, 5000, Cmp::Particle::kWorldScalePreset );
  SPDLOG_DEBUG( "Altar activated to state {}.", altar_cmp.get_sacrifice_count() );
  play_sacrifice_anim( altar_cmp, SacrificeAnimType::DEFAULT );

  if ( altar_cmp.get_sacrifice_count() == altar_cmp.get_exitkey_drop_threshold() and not altar_cmp.is_exitkey_lockout() )
  {
    drop_reward_item( "item.exitkey" );
    altar_cmp.set_exitkey_lockout();
  }
}

void AltarSystem::sacrifice_exitkey( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid )
{
  // all the relics must be sacrificed first
  const uint8_t sacrifice_count = altar_cmp.get_sacrifice_count();
  if ( sacrifice_count < altar_cmp.get_exitkey_drop_threshold() or sacrifice_count >= altar_cmp.get_cryptkey_drop_threshold() ) return;

  reg().patch<Cmp::Altar::MultiBlock>( altar_entity, []( Cmp::Altar::MultiBlock &cmp ) { cmp.increment_sacrifice_count(); } );

  // turn this altar's candle flames blue
  for ( auto [particle_entt, particle_cmp, particle_uuid_cmp] : reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
  {
    if ( particle_uuid_cmp != altar_uuid ) continue;
    if ( not particle_cmp.sprite->get_tag().contains( "candle" ) ) continue;
    auto *flame_particle = dynamic_cast<Cmp::Particle::FlameParticleSprite *>( particle_cmp.sprite.get() );
    if ( flame_particle ) { flame_particle->m_final_flame_color = { 0, 0, 255 }; }
  }
  SPDLOG_DEBUG( "Altar activated to state {}.", altar_cmp.get_sacrifice_count() );
  play_sacrifice_anim( altar_cmp, SacrificeAnimType::KEY );

  if ( altar_cmp.get_sacrifice_count() >= altar_cmp.get_cryptkey_drop_threshold() and not altar_cmp.is_cryptkey_lockout() )
  {
    drop_reward_item( "item.cryptkey" );
    altar_cmp.set_cryptkey_lockout();
  }
}

void AltarSystem::play_sacrifice_anim( const Cmp::Altar::MultiBlock &altar_cmp, SacrificeAnimType sacrifice_anim_type )
{
  Sys::SpriteKey sprite_type = ( sacrifice_anim_type == SacrificeAnimType::KEY ) ? "sprite.graveyard.altar.key.anim"
                                                                                 : "sprite.graveyard.altar.relic.anim";

  float altar_sacrifice_anim_height = Sys::SpriteStore::instance().get( sprite_type ).sprite_size().y;
  // get the center (topleft coord), then adjust to center the altar_sacrifice_anim, then adjust for altar_sacrifice_anim height
  Cmp::Position new_pos( altar_cmp.getCenter() - sf::Vector2{ 8.f, 4.f } - sf::Vector2{ 0.f, altar_sacrifice_anim_height }, Constants::kGridSizePxF );
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
