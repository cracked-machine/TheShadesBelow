#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteAlpha.hpp>
#include <Components/Altar/Activated.hpp>
#include <Components/Altar/AltarActivationTimer.hpp>
#include <Components/Altar/BloodCount.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/Altar/Offering.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/DestroyedObstacle.hpp>
#include <Components/Inventory/FlashUIExtraLife.hpp>
#include <Components/Inventory/FlashUIInventory.hpp>
#include <Components/Inventory/FlashUIWealth.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/LootContainer.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/SummonOffering.hpp>
#include <Components/Particle/SpriteOwner.hpp>
#include <Components/Persistent/AltarBloodQuota.hpp>
#include <Components/Persistent/AltarOfferingCooldown.hpp>
#include <Components/Persistent/AltarSummonOfferingCooldown.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/ExtraLife.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Player/ToxicImmunity.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/SceneSettings/CurrentScene.hpp>
#include <Components/Stats/ActionReward.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/OfferingAction.hpp>
#include <Components/Stats/PlayerStats.hpp>
#include <Components/Stats/SideEffects.hpp>
#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Venom.hpp>
#include <Components/Toxicity/Vertigo.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/AddInventoryEvent.hpp>
#include <Events/AltarActivatedEvent.hpp>
#include <Events/AltarApproachedEvent.hpp>
#include <Events/AltarBloodOfferedEvent.hpp>
#include <Events/BloodRefusedEvent.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/DestroyNpcEvent.hpp>
#include <Events/KeyGraveExhumedEvent.hpp>
#include <Events/ItemPickedUpEvent.hpp>
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
#include <vector>

namespace Game::Sys
{

AltarSystem::AltarSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&AltarSystem::on_player_action>( this );
  std::ignore = get_systems_event_queue().sink<Events::KeyGraveExhumedEvent>().connect<&AltarSystem::on_key_grave_exhumed>( this );
}

void AltarSystem::on_key_grave_exhumed( [[maybe_unused]] const Events::KeyGraveExhumedEvent &event )
{
  // reset the activated altar to unlit, so it takes its blood quota again for another key grave
  for ( auto [altar_entt, blood_cmp, altar_uuid] : reg().view<Cmp::Altar::Activated, Cmp::Altar::BloodCount, Cmp::UUID>().each() )
  {
    blood_cmp.reset();
    // put out its centre column; an extra life's particles on the same altar are left alone.
    // Collected first, so the particle view is not modified while it is iterated
    std::vector<entt::entity> particle_entts;
    for ( auto [particle_entt, particle_cmp, particle_uuid] : reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
    {
      if ( particle_uuid != altar_uuid ) continue;
      if ( particle_cmp.sprite->get_tag() != Factory::Altar::kActivatedParticleTag ) continue;
      particle_entts.push_back( particle_entt );
    }
    reg().destroy( particle_entts.begin(), particle_entts.end() );
    if ( auto *anim_cmp = reg().try_get<Cmp::AnimData>( altar_entt ) )
    {
      anim_cmp->m_sprite_type = Factory::Altar::get_altar_sprite( anim_cmp->m_sprite_type, false );
    }
    SPDLOG_INFO( "Key grave exhumed, altar {} reset", static_cast<uint32_t>( altar_entt ) );
  }
  reg().clear<Cmp::Altar::Activated>();
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
  if ( Utils::is_current_scene( reg(), Cmp::SceneSettings::SceneId::GRAVEYARD ) )
  {
    update_mb_zorder<Cmp::Altar::MultiBlock, Cmp::Altar::Segment>();
    if ( auto *altar_timer = reg().try_get<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) ) ) *altar_timer += dt;
    check_player_approached_altar();
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
}

void AltarSystem::check_player_approached_altar()
{
  // only a graveyard altar still waiting for blood asks for it
  std::optional<sf::FloatRect> near_altar;
  const int quota = Sys::PersistSystem::get<Cmp::Persist::AltarBloodQuota>( reg() ).get_value();
  for ( auto [altar_entt, altar_cmp, blood_cmp] : reg().view<Cmp::Altar::MultiBlock, Cmp::Altar::BloodCount>().each() )
  {
    if ( altar_cmp.get_kind() != Cmp::Altar::Kind::GRAVEYARD or reg().all_of<Cmp::Altar::Activated>( altar_entt ) or blood_cmp.is_full( quota ) )
      continue;
    if ( Utils::Player::is_player_near( reg(), altar_cmp ) )
    {
      near_altar = sf::FloatRect( altar_cmp.position, altar_cmp.size );
      break;
    }
  }
  // once per approach, not every frame the player stays near
  if ( near_altar and not m_player_near_altar ) get_systems_event_queue().trigger( Events::AltarApproachedEvent{ *near_altar } );
  m_player_near_altar = near_altar.has_value();
}

bool AltarSystem::check_player_altar_offering( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp )
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
    if ( *altar_timer < timeout ) return false;
  }

  auto *altar_uuid_cmp = reg().try_get<Cmp::UUID>( altar_entity );
  if ( not altar_uuid_cmp ) throw std::runtime_error( "Altar does not have UUID" );

  // an item the altar accepts is offered, otherwise a held blood vial is poured into a graveyard altar
  return try_offer_held_item( altar_entity, altar_cmp, *altar_uuid_cmp ) or offering_blood_vial( altar_entity, altar_cmp, *altar_uuid_cmp );
}

bool AltarSystem::try_offer_held_item( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid )
{
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type == "" ) return false;
  const auto &item = Sys::ItemStore::instance().get( inventory_type );
  if ( not item.is_offerable() ) return false;

  auto is_carrying_tool = [&]() { return item.item_type.contains( "item.shovel" ) or item.item_type.contains( "item.dowsingrod" ); };

  // only spent tools can be offered
  if ( is_carrying_tool() and Utils::Player::get_inventory_wear_level( reg() ) > 0 ) { return false; }

  // the altar refuses an offering whose NPC it has already summoned up to max_instances, otherwise the item is consumed for nothing
  if ( const auto spawn_npc = item.get_offering_npc_spawn_effect();
       spawn_npc and spawn_npc->max_instances > 0 and Utils::Npc::count_spawned_by( reg(), altar_uuid ) >= spawn_npc->max_instances )
  {
    return false;
  }

  // an offering whose rewards are all for another kind of altar is refused (e.g. nightshade at the crypt altar)
  const auto reward = item.get_offering_reward( altar_cmp.get_kind() );
  if ( not reward and not item.get_rewards<Cmp::OfferingAction>().empty() ) return false;

  // the altar may refuse the offering (e.g. a relic once the exitkey has dropped), so only consume the item once it has accepted
  if ( not grant_offering_reward( altar_entity, altar_cmp, altar_uuid, reward ) ) return false;

  SPDLOG_INFO( "Player made an offering: {}", item.item_type );
  const auto offering_action = Utils::Player::apply_action_from_item_store<Cmp::OfferingAction>( reg(), item.item_type );
  play_action_sfx( offering_action );
  trigger_side_effects( offering_action, Cmp::Position( altar_cmp.position, altar_cmp.size ), altar_entity );
  Factory::Player::destroy_inventory( reg(), item.item_type );
  // the exit key has left the inventory, so the crypt key can take its place
  if ( reward and reward->altar == Cmp::ActionReward::AltarEffect::CRYPT_KEY ) give_reward_key( "item.cryptkey" );
  // the offering has left the inventory, so an inventory reward item can take its place
  if ( reward and not reward->item.empty() )
  {
    // items dropped by a summoning offering are bound to the summoned NPC, e.g. Azazel stays until its jewelry is offered
    std::function<void( entt::entity )> on_created;
    if ( offering_action.side_effects().spawn_npc )
    {
      on_created = [this, summoner = altar_uuid]( entt::entity item_entt )
      { reg().emplace_or_replace<Cmp::Npc::SummonOffering>( item_entt, summoner ); };
    }
    offering_for_item( altar_cmp, *reward, on_created );
  }
  return true;
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
    case AltarEffect::BANISH:
      offering_for_banish( altar_cmp );
      return true;
    case AltarEffect::EXTRA_LIFE:
      return offering_for_extra_life( altar_entity, altar_cmp, altar_uuid );
    case AltarEffect::CRYPT_KEY:
      offering_cryptkey( altar_cmp );
      return true;
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
                                                                entt::null, on_created, /*sting=*/true, /*player_reward=*/true ) );
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

  // only the crypt altar grants an extra life; a graveyard altar's active sprite shows its key grave hunt instead
  if ( altar_cmp.get_kind() != Cmp::Altar::Kind::CRYPT ) return false;
  if ( not Utils::Player::get_projected_position( reg() ).findIntersection( altar_cmp ) ) return false;

  // the player respawns at the crypt start position
  Cmp::Player::ExtraLife extra_life;
  extra_life.altar_uuid = altar_uuid;

  // the altar stays lit while the extra life is held, see Factory::Player::remove_player_extra_life()
  activate_altar( altar_entity, altar_cmp, altar_uuid, Factory::Altar::kExtraLifeParticleTag );
  // both go on the player so they transfer between scenes
  reg().emplace_or_replace<Cmp::FlashUIExtraLife>( Utils::Player::get_entity( reg() ) );
  reg().emplace_or_replace<Cmp::Player::ExtraLife>( Utils::Player::get_entity( reg() ), extra_life );
  m_sound_bank.get_effect( "witches_jar_offering" ).play();
  return true;
}

bool AltarSystem::offering_blood_vial( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid )
{
  if ( altar_cmp.get_kind() != Cmp::Altar::Kind::GRAVEYARD ) return false;
  auto *blood_cmp = reg().try_get<Cmp::Altar::BloodCount>( altar_entity );
  if ( not blood_cmp ) return false;
  // the player must face the altar, not just stand near it
  if ( not Utils::Player::get_projected_position( reg() ).findIntersection( altar_cmp ) ) return false;
  if ( Utils::Player::get_mortality( reg() ).state != Cmp::Player::Mortality::State::ALIVE ) return false;

  // the altar only takes blood poured from a vial held in the selected slot
  const auto slot_entt = Utils::Player::get_inventory_entt( reg() );
  if ( slot_entt == entt::null ) return false;
  const auto &vial = *reg().get<Cmp::PlayerInventorySlot>( slot_entt ).m_item;
  if ( not vial.is_blood() ) return false;
  const int blood = vial.get_blood_heal();
  if ( blood <= 0 ) return false;

  const int quota = Sys::PersistSystem::get<Cmp::Persist::AltarBloodQuota>( reg() ).get_value();
  if ( blood_cmp->is_full( quota ) ) return false;

  const sf::FloatRect altar_bounds( altar_cmp.position, altar_cmp.size );
  // once an altar is activated its key grave must be exhumed first, so no altar takes blood until then
  if ( not reg().view<Cmp::Altar::Activated>().empty() )
  {
    SPDLOG_INFO( "An altar is already activated, refusing blood until its key grave is exhumed" );
    get_systems_event_queue().trigger( Events::BloodRefusedEvent{ altar_bounds } );
    return false;
  }

  // pour one vial of the held stack into the altar
  const Sys::ItemKey vial_type = vial.item_type;
  Factory::Player::take_one_from_slot( reg(), slot_entt );
  // signal UI to flash
  reg().emplace_or_replace<Cmp::FlashUIInventory>( reg().create() );
  m_sound_bank.get_effect( "shrine_lighting" ).play();

  blood_cmp->add( blood, quota );
  SPDLOG_INFO( "Player poured {} ({} blood) into altar (blood {} of {})", vial_type.str(), blood, blood_cmp->value(), quota );
  play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
  get_systems_event_queue().trigger( Events::AltarBloodOfferedEvent{ altar_bounds } );

  if ( blood_cmp->is_full( quota ) ) activate_graveyard_altar( altar_entity, altar_cmp, altar_uuid );
  return true;
}

void AltarSystem::activate_graveyard_altar( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid )
{
  // light the centre column
  activate_altar( altar_entity, altar_cmp, altar_uuid, Factory::Altar::kActivatedParticleTag );
  reg().emplace_or_replace<Cmp::Altar::Activated>( altar_entity );
  SPDLOG_INFO( "Altar {} activated", static_cast<uint32_t>( altar_entity ) );
  // GraveSystem picks the key grave and sends a crow to guide the player to it
  get_systems_event_queue().trigger( Events::AltarActivatedEvent{ Cmp::Position( altar_cmp.position, altar_cmp.size ) } );
}

void AltarSystem::offering_cryptkey( const Cmp::Altar::MultiBlock &altar_cmp )
{
  play_offering_anim( altar_cmp, OfferingAnimType::KEY );
  m_sound_bank.get_effect( "crypt_altar_activate" ).play();
}

void AltarSystem::activate_altar( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid,
                                  const char *particle_tag )
{
  auto *anim_cmp = reg().try_get<Cmp::AnimData>( altar_entity );
  if ( not anim_cmp ) return;
  const Sys::SpriteKey active_sprite_type = Factory::Altar::get_altar_sprite( anim_cmp->m_sprite_type, true );
  if ( anim_cmp->m_sprite_type == active_sprite_type ) return;

  anim_cmp->m_sprite_type = active_sprite_type;
  const sf::Vector2f particle_offset = ( altar_cmp.get_kind() == Cmp::Altar::Kind::CRYPT ) ? sf::Vector2f{ 8.f, 24.f } : sf::Vector2f{ 40.f, 21.f };
  Factory::Particle::add_crypt_altar_ps( m_reg, particle_tag, 0.5, 25.f, altar_uuid, altar_cmp.position + particle_offset, 5000 );
  m_sound_bank.get_effect( "crypt_altar_activate" ).play();
}

void AltarSystem::play_offering_anim( const Cmp::Altar::MultiBlock &altar_cmp, OfferingAnimType offering_anim_type )
{
  Sys::SpriteKey sprite_type = ( offering_anim_type == OfferingAnimType::KEY ) ? "sprite.graveyard.altar.key.anim"
                                                                               : "sprite.graveyard.altar.relic.anim";

  float altar_offering_anim_height = Sys::SpriteStore::instance().get( sprite_type ).sprite_size().y;
  // get the center (topleft coord), then adjust to center the altar_offering_anim, then adjust for altar_offering_anim height
  Cmp::Position new_pos( altar_cmp.getCenter() - sf::Vector2{ 7.f, 28.f } - sf::Vector2{ 0.f, altar_offering_anim_height }, Constants::kGridSizePxF );
  Factory::Altar::create_altar_offering_anim( reg(), new_pos, sprite_type );

  reg().emplace_or_replace<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) );
}

void AltarSystem::give_reward_key( const Sys::ItemKey &item_key )
{
  get_systems_event_queue().trigger( Events::AddInventoryEvent( item_key ) );
  m_sound_bank.get_effect( "equip_inventory" ).play();
  // signal UI to flash
  reg().emplace_or_replace<Cmp::FlashUIInventory>( reg().create() );
  // enqueued rather than triggered, as the caller may still be iterating registry views
  get_systems_event_queue().enqueue( Events::ItemPickedUpEvent( item_key ) );
  SPDLOG_INFO( "Altar granted {}", item_key.str() );
}

} // namespace Game::Sys
